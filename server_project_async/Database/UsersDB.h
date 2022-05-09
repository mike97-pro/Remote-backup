//
// Created by mike_ on 26/07/2020.
//

#ifndef PDS_PROJECT_2020_SERVER_USERSDB_H
#define PDS_PROJECT_2020_SERVER_USERSDB_H
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <optional>
#include <memory>
#include <mutex>
#include "User.h"


class UsersDB {
    std::unordered_map<std::string, User> users;
    std::fstream DB;
    const std::string DB_PATH = "../Database/usersfiles/";
    static std::mutex m;
    UsersDB();
public:
    static std::shared_ptr<UsersDB> get_DB();
    int insert_user(const User &user);
    std::optional<User> get_user(const std::string& email);
    int remove_user(const std::string& email);
    int change_email(const std::string& oldEmail, const std::string& newEmail);
    int change_nick(const std::string& email, const std::string& nick);
    int change_password(const std::string& email, const std::string& pw);
    int set_path(const std::string& email, const std::string& pth);
    void reset_DB();

};
static std::shared_ptr<UsersDB> usersDb;

#endif //PDS_PROJECT_2020_SERVER_USERSDB_H
