//
// Created by mentz on 27/08/20.
//

#ifndef CLIENT_PROJECT_OUT_H
#define CLIENT_PROJECT_OUT_H

#include <iostream>
#include <mutex>

static std::mutex mut;

class Out {
public:
    Out()=default;
    void operator() (const std::string& s) {
        std::lock_guard<std::mutex> l(mut);
        std::cout<<s<<std::endl;
    }
};


#endif //CLIENT_PROJECT_OUT_H
