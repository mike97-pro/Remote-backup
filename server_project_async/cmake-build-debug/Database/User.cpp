//
// Created by mike_ on 26/07/2020.
//
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include "User.h"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"

std::string User::get_email() const{
    return email;
}

std::string User::get_nickname() {
    return nickname;
}

std::string User::get_password() {
    return password;
}

void User::set_email(const std::string &em) {
    email = em;
}

void User::set_nickname(const std::string &nick) {
    nickname = nick;
}

void User::set_password(const std::string &pw) {
    password = pw;
}

std::vector<char> User::serialize() const {
    boost::property_tree::ptree pt;
    pt.put("email",email);
    pt.put("nick",nickname);
    pt.put("pw",password);
    pt.put("path",observedPath);
    std::ostringstream buf;
    boost::property_tree::write_json(buf,pt);
    std::string s = buf.str();
    return std::vector<char>(s.begin(),s.end());
}

void User::deserialize(std::vector<char> &c) {
    boost::property_tree::ptree pt;
    std::stringstream buf;
    std::string s(c.begin(),c.end());
    buf<<s;
    boost::property_tree::read_json(buf,pt);
    email = pt.get<std::string>("email");
    nickname = pt.get<std::string>("nick");
    password = pt.get<std::string>("pw");
    observedPath = pt.get<std::string>("path");
}

std::string User::get_path() {
    return observedPath;
}

void User::set_path(const std::string &pth) {
    observedPath = pth;
}


