//
// Created by franc on 28/04/2020.
//

#ifndef SERVER_PROJECT_BASE_H
#define SERVER_PROJECT_BASE_H

#include <string>
#include <iostream>
#include <chrono>
#include <boost/filesystem.hpp>
#include <mutex>
namespace fs=boost::filesystem;

class Base {
protected:
    std::recursive_mutex m;
    std::string name;
    time_t  last_mod_on_client;
    time_t last_sync_on_server;

public:
    Base (){
        //std::cout<<"Constructor "<<this<<std::endl;
    }
    virtual ~Base(){
        //std::cout<<"Destructor "<<this<<std::endl;
    }

    const std::string& getName() {
        std::lock_guard<std::recursive_mutex> l(this->m);
        return this->name;
    }

    void setlast_mod(time_t time){
        std::lock_guard<std::recursive_mutex> l(this->m);
        last_mod_on_client=time;
    }

    void setlast_sync(time_t time){
        std::lock_guard<std::recursive_mutex> l(this->m);
        last_sync_on_server=time;
    }

    virtual int mType() = 0;

    virtual void ls(int indent) = 0;

    bool synch() {
        std::lock_guard<std::recursive_mutex> l(this->m);
        return this->last_sync_on_server >= this->last_mod_on_client;
    }

    const time_t getlast_mod_on_client(){
        std::lock_guard<std::recursive_mutex> l(this->m);
        return last_mod_on_client;
    }


    const time_t getlast_sync_on_server(){
        std::lock_guard<std::recursive_mutex> l(this->m);
        return last_sync_on_server;
    }
};

#endif //SERVER_PROJECT_BASE_H
