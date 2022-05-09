//
// Created by mentz on 29/07/20.
//

#ifndef CLIENT_PROJECT_MESSAGE_H
#define CLIENT_PROJECT_MESSAGE_H

#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <string>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <optional>
#include <atomic>
#include "Out.h"

class Socket;
namespace fs=boost::filesystem;

class Message {
public:
    Message(const std::string& name, const std::string& action, std::optional<std::string> chekcsum, int type);
    std::string getMessage();
    void consume(boost::shared_ptr<Socket>& soc, std::atomic<bool>& running);
    static Message endVerify();
    static Message startVerify(std::string name);
private:
    std::string message;
    std::string name;
    std::string action;
    int type;
};


#endif //CLIENT_PROJECT_MESSAGE_H
