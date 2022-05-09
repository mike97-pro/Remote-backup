//
// Created by franc on 05/05/2020.
//

#ifndef CLIENT_PROJECT_FILE_H
#define CLIENT_PROJECT_FILE_H


#include "Base.h"

class File : public Base{
    size_t size;
    friend class Dir;
    std::string checksum;
public:
    File(std::string name, size_t size, time_t time);
    uintmax_t getSize();
    int mType() override ;
    void ls(int indent) override;
    void setSize(uintmax_t size);
    const std::string& getCheck();
    void setCheck();
};


#endif //CLIENT_PROJECT_FILE_H
