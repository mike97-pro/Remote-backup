//
// Created by mike_ on 31/07/2020.
//

#ifndef PDS_PROJECT_2020_SERVER_QUEUE_H
#define PDS_PROJECT_2020_SERVER_QUEUE_H
#include <optional>
#include <queue>
#include <mutex>
#include <condition_variable>

template <class T>
class Queue {
    std::queue<T> jobs;
    std::mutex m;
    std::condition_variable cv_producers, cv_consumers;
    int maxSize;
    bool done;
public:
    Queue(int maxS) : maxSize(maxS), done(false){};

    void put(T& job);

    std::optional<T> get();

    void finish();

    int size();

    bool ended();

    bool empty();

    void start();
};

template <class T>
void Queue<T>::put(T& job) {
    std::unique_lock lock(m);
    cv_producers.wait(lock,[this](){return jobs.size() <= maxSize;});

    jobs.push(job);

    cv_consumers.notify_one();
}

template <class T>
std::optional<T> Queue<T>::get() {
    std::unique_lock lock(m);
    cv_consumers.wait(lock,[this](){return (!jobs.empty() || done);});
    if(!jobs.empty()){
        auto job = jobs.front();
        jobs.pop();
        cv_producers.notify_one();
        return job;
    }

    return std::optional<T>();
}

template<class T>
void Queue<T>::finish() {
    std::lock_guard<std::mutex> lock(m);
    done = true;
    cv_consumers.notify_all();
    cv_producers.notify_all();
}

template<class T>
void Queue<T>::start() {
    std::lock_guard<std::mutex> lock(m);
    done = false;
}

template<class T>
bool Queue<T>::ended(){
    return done;
}

template<class T>
bool Queue<T>::empty(){
    return jobs.empty();
}

template<class T>
int Queue<T>::size() {
    return jobs.size();
}

#endif //PDS_PROJECT_2020_SERVER_QUEUE_H
