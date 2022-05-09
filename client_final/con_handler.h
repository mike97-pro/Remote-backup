//
// Created by mentz on 19/08/20.
//

#ifndef CLIENT_PROJECT_CON_HANDLER_H
#define CLIENT_PROJECT_CON_HANDLER_H

#include <iostream>
#include <exception>
#include <thread>
#include <future>
#include "FileSystem/Dir.h"
#include "FileSystem/FileWatcher.h"
#include "Socket.h"
#include <mutex>

class con_handler {
    boost::shared_ptr<Socket> soc;
    static void start_sync(std::string s, std::atomic<bool>& running, boost::shared_ptr<Socket>& soc,
            std::shared_ptr<Jobs<Message>> messagges, std::atomic<bool>& con_problems, std::shared_ptr<Dir>& root);
    status con_status;
    std::shared_ptr<Jobs<Message>> messagges;
    std::shared_ptr<Dir> root;
    std::string name;
    int port;
public:
    con_handler(boost::asio::io_context& io_context, std::string name, std::string port);
    void start(boost::asio::io_context& io_context);
    void connected();
    void log_in(int& retry, std::string& answer, std::string& email, std::string& pw, std::string& path);
    void sign_in(int& retry, std::string& answer, std::string& email, std::string& pw);
    void logged(std::future<void>& asy, std::future<void>& f, std::string& answer, boost::asio::io_context& io_context,
                bool& in_synch, std::atomic<bool>& running, int& retry, std::atomic<bool>& con_problems, std::string& email, std::string& pw, std::string& path);
    ~con_handler();
};


#endif //CLIENT_PROJECT_CON_HANDLER_H
