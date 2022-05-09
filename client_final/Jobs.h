//
// Created by mentz on 29/07/20.
//

#ifndef CLIENT_PROJECT_JOBS_H
#define CLIENT_PROJECT_JOBS_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <optional>

template <class T>
class Jobs {
    std::queue<T> buffer;
    std::mutex m;
    std::condition_variable cv_consumer;
    std::condition_variable cv_producer;
    int size;
    std::atomic<bool> quit;
public:
    Jobs(int n):size(n){quit.store(false);}

    void put(T& obj){
        std::unique_lock ul(m);
        cv_producer.wait(ul, [this](){return (this->buffer.size()<this->size || this->quit==true);});
        if(this->quit.load())
            return;
        buffer.push(obj);
        cv_consumer.notify_one();
    }

    std::optional<T> get(){
        std::optional<T> value;
        std::unique_lock ul(m);
        cv_consumer.wait(ul, [this](){return !this->buffer.empty() || this->quit==true;});
        if(this->quit.load())
            return value;
        value=buffer.front();
        buffer.pop();
        cv_producer.notify_all();
        return value;
    }

    void finish(){
        quit.store(true);
        cv_consumer.notify_all();
        cv_producer.notify_all();
    }

    void restart(){
        quit.store(false);
    }
};

#endif //CLIENT_PROJECT_JOBS_H
