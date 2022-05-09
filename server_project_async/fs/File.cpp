//
// Created by franc on 05/05/2020.
//

#include "File.h"
#include <iostream>
#include "../hashing/MD5.h"

File::File(std::string name, uintmax_t size, time_t time): size(size){
    this->name=name;
    this->last_mod_on_client=0;
    this->last_sync_on_server=time;
    this->setCheck();
};

uintmax_t File::getSize() {
    std::lock_guard<std::recursive_mutex> l(this->m);
    return size;
}

void File::setSize(uintmax_t size) {
    std::lock_guard<std::recursive_mutex> l(this->m);
    this->size=size;
}

void File::ls(int indent) {
    std::lock_guard<std::recursive_mutex> l(this->m);
    for(int i=0;i<indent;i++){
        std::cout<<" ";
    }
    std::cout<<this->name<<" "<< this->size <<" "<<std::ctime(&this->last_mod_on_client)<<std::endl;
}

int File::mType() {
    std::lock_guard<std::recursive_mutex> l(this->m);
    return 1; //Dir 0 - File 1
}

const std::string &File::getCheck() {
    std::lock_guard<std::recursive_mutex> l(this->m);
    return this->checksum;
}

void File::setCheck() {
    std::lock_guard<std::recursive_mutex> l(this->m);
    std::ifstream ff(this->name, std::ios::binary);
    size_t read=0;
    size_t to_read;
    MD5 md;
    char buf[1024];
    while(read<size) {
        to_read=size-read;
        if(to_read>1024)
            to_read=1024;
        ff.read(buf, to_read);
        md.update(buf, to_read);
        read+=to_read;
    }
    ff.close();
    md.finalize();
    this->checksum=md.hexdigest();
}