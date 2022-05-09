//
// Created by mike_ on 26/07/2020.
//

#ifndef PDS_PROJECT_2020_SERVER_USER_H
#define PDS_PROJECT_2020_SERVER_USER_H
#include <iostream>
#include <vector>
#include "../hashing/SHA256.h"

class User {
    std::string email, nickname, password, observedPath;
public:
    User(){};
    User (const std::string& em, const std::string& nick, const std::string& pw) : email(em), nickname(nick), password(pw){};
    std::string get_email() const;
    std::string get_nickname();
    std::string get_password();
    std::string get_path();
    void set_email(const std::string& em);
    void set_nickname(const std::string& nick);
    void set_password(const std::string& pw);
    void set_path(const std::string &pth);
    std::vector<char> serialize() const;
    void deserialize(std::vector<char>& c);
};


#endif //PDS_PROJECT_2020_SERVER_USER_H
