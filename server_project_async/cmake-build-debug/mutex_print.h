//
// Created by mike_ on 13/09/2020.
//

#include <iostream>
#include <mutex>

#ifndef PDS_PROJECT_2020_SERVER_MUTEX_PRINT_H
#define PDS_PROJECT_2020_SERVER_MUTEX_PRINT_H


class mutex_print{
    inline static std::mutex m;
public:
    mutex_print(){};
    void operator()(const std::string& s){
        std::lock_guard<std::mutex> l(m);
        std::cout<<s<<std::endl;
    }
};


#endif //PDS_PROJECT_2020_SERVER_MUTEX_PRINT_H
