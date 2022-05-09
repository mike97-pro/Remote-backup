//
// Created by mike_ on 29/07/2020.
//

#ifndef PDS_PROJECT_2020_SERVER_CON_HANDLER_H
#define PDS_PROJECT_2020_SERVER_CON_HANDLER_H
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <iostream>
#include "Queue.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "Database/UsersDB.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "mutex_print.h"
#include "Logger/Logger.h"

using namespace boost::asio;
using ip::tcp;
using std::cout;
using std::endl;

#define PATH "../UsersBackup"

typedef enum{CONNECTED, LOG_IN, SIGN_IN, LOGGED, CLOSE} status;

class con_handler : public boost::enable_shared_from_this<con_handler>
{
private:
    tcp::socket sock;
    std::string message="Hello From Server!";
    enum { max_length = 1024 };
    size_t sizeOfdata;
    std::vector<char> data;
    status con_status;
    std::shared_ptr<UsersDB> usersDB;
    User owner;
    Queue<std::vector<char>> commands;
    mutex_print printer;
    std::shared_ptr<Logger> log;

    void sync();
    void finish_syncing();
public:
    typedef boost::shared_ptr<con_handler> pointer;
    con_handler(boost::asio::io_service& io_service): sock(io_service), con_status(CONNECTED), commands(100){
        usersDB = UsersDB::get_DB();
        log = Logger::getLogger();
    }
// creating the pointer
    static pointer create(boost::asio::io_service& io_service);
//socket creation
    tcp::socket& socket();

    void start();

    void handle_read_command_size(const boost::system::error_code& err, size_t bytes_transferred);

    void async_write(tcp::socket& socket, const std::string& buffer);

    void handle_write(const boost::system::error_code& err, size_t bytes_transferred);

    void async_read_command();

    void handle_read_command(const boost::system::error_code &err, size_t bytes_transferred);

    void send_backup(std::string& server_path);
};


#endif //PDS_PROJECT_2020_SERVER_CON_HANDLER_H
