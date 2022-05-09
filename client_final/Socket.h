//
// Created by mentz on 29/07/20.
//

#ifndef CLIENT_PROJECT_SOCKET_H
#define CLIENT_PROJECT_SOCKET_H

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <utility>
#include <vector>
#include <iostream>
#include "hashing/SHA256.h"
#include "Message.h"
#include "Jobs.h"

using boost::asio::ip::tcp;

typedef enum{CONNECTED, LOG_IN, SIGN_IN, LOGGED, CLOSE} status;

class Socket : public boost::enable_shared_from_this<Socket> {
    //boost::asio::io_context& io_context;
    tcp::socket socket;

    Socket (const Socket& )=delete;
    Socket &operator=(const Socket&)=delete;

    Socket(boost::asio::io_context& io_context): socket(io_context){}

public:
    size_t sizeOfdata;
    std::vector<char> data;
    std::shared_ptr<Jobs<Message>> mes;

    // creating the pointer
    static boost::shared_ptr<Socket> create(boost::asio::io_context& io_service)
    {
        return boost::shared_ptr<Socket>(new Socket(io_service));
    }

    ~Socket(){
        this->close();
    }

    Socket (Socket&& other): socket(std::move(other.socket)){}

    Socket &operator=(Socket &&other){
        if(socket.is_open()) socket.close();
        socket=std::move(other.socket);
        return *this;
    }

    void close(){
        socket.close();
    }

    void connect(const std::string& name, int port){
        tcp::resolver resolver(static_cast<boost::asio::io_context &>(socket.get_executor().context()));
        tcp::resolver::results_type endpoints = resolver.resolve(name, std::to_string(port));
        boost::asio::connect(socket, endpoints);
    }

    std::string read(){
        boost::system::error_code error;
        size_t size;
        boost::asio::read(socket, boost::asio::buffer(&size, sizeof(size_t)), error);
        if(error){
            throw boost::system::system_error(error); // Some error.
        }
        std::vector<char> v(size);
        boost::asio::read(socket, boost::asio::buffer(v,size), error);
        if(error){
            throw boost::system::system_error(error); // Some error.
        }
        v.push_back('\0');
        std::string s(v.data());
        return s;
    }


    void write(const std::string& buffer){
        size_t size=buffer.size();
        boost::system::error_code error;
        boost::asio::write(socket, boost::asio::buffer(&size, sizeof(size_t)), error);
        if(error) {
            throw boost::system::system_error(error);
        }
        boost::asio::write(socket, boost::asio::buffer(buffer, size), error);
        if(error) {
            throw boost::system::system_error(error);
        }
    }

    void write_file(const std::string& p, std::atomic<bool>& running){
        size_t size=boost::filesystem::file_size(p);
        size_t read=0;
        std::ifstream f(p, std::ios::binary);
        size_t to_read;
        boost::array<char, 1024> buf;
        boost::system::error_code error;
        while(read<size && running.load()) {
            to_read=size-read;
            if(to_read>1024)
                to_read=1024;
            f.read(buf.data(), to_read);
            read+=to_read;
            boost::asio::write(socket, boost::asio::buffer(&to_read, sizeof(to_read)), error);
            /*if(error) { //non serve che siano lanciate eccezioni perchè le rileva già io_context nel ciclo infinito della read
                throw boost::system::system_error(error);
            }*/
            boost::asio::write(socket, boost::asio::buffer(buf, to_read), error);
            /*if(error) {
                throw boost::system::system_error(error);
            }*/
        }
        f.close();
    }

    void read_file(boost::property_tree::ptree& pt, std::string& abs_path){
        int num_pac = pt.get<size_t>("size") / 1024;
        std::ofstream f(abs_path, std::ios::binary);
        size_t size;
        boost::system::error_code error;
        if(pt.get<size_t>("size")!=0) {
            for (int i = 0; i <= num_pac; i++) {
                boost::asio::read(socket, boost::asio::buffer(&size, sizeof(size_t)), error);
                if(error==boost::asio::error::eof) {
                    throw boost::system::system_error(error); // Some other error.
                }
                std::vector<char> v(size);
                boost::asio::read(socket, boost::asio::buffer(v,size), error);
                f.write(v.data(), v.size());
            }
        }
        f.close();
    }

    void async_write(const std::string& buffer) {
        std::size_t size=buffer.size();
        boost::asio::async_write(socket, boost::asio::buffer(&size, sizeof(size_t)),
                boost::bind(&Socket::handle_write,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
        boost::asio::async_write(socket,
                boost::asio::buffer(buffer, size),
                boost::bind(&Socket::handle_write,
                            shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
    }

    void handle_write(const boost::system::error_code& err, size_t bytes_transferred)
    {
        if (!err) { //non serve che lanci un'eccezione perchè è già verificata dalla async read command
            //std::cout << "Ho scritto"<< std::endl;
        }
    }

    void setMes(std::shared_ptr<Jobs<Message>> m){
        mes=m;
    }

    void async_read_command(){
        boost::asio::async_read(socket,
                boost::asio::buffer((char *) &sizeOfdata, sizeof(size_t)),
                boost::bind(&Socket::handle_read_command_size,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }

    void handle_read_command_size(const boost::system::error_code &err, size_t bytes_transferred) {
        if (!err) {
            //std::cout<<sizeOfdata<<std::endl;
            data.resize(sizeOfdata);
            boost::asio::async_read(socket,
                                    boost::asio::buffer(data, sizeOfdata),
                                    boost::bind(&Socket::handle_read_command,
                                                shared_from_this(),
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred));
        } else {
            throw boost::system::system_error(err);
        }
    }

    void handle_read_command(const boost::system::error_code &err, size_t bytes_transferred) {
        if (!err) {
            data.push_back('\0');
            //std::cout<<data.data()<<std::endl;
            if(data[0]=='0' && data.size()==2){
                static_cast<boost::asio::io_context &>(socket.get_executor().context()).stop();
                return;
            }
            boost::property_tree::ptree pt;
            std::stringstream buf;
            buf.str(data.data());
            boost::property_tree::read_json(buf, pt);
            fs::path path(Dir::getRoot()->getName());
            auto p=path.parent_path().string()+pt.get<std::string>("path");
            if(pt.get<std::string>("outcome")=="0") {
                if(fs::is_directory(p)) {
                    Message m(p, "A", std::nullopt, 0);
                    mes->put(m);
                }
                else {
                    Message m(p, "A", std::nullopt, 1);
                    mes->put(m);
                }
            } else {
                Out out;
                auto last_sync=pt.get<time_t>("last_sync");
                out(p+" is synchronized at "+std::ctime(&last_sync)+"Premere 0 per tornare al menu.");
                (Dir::getRoot()->get(p))->setlast_sync(last_sync);
            }
            async_read_command();
        } else {
            throw boost::system::system_error(err);
        }
    }
};

#endif //CLIENT_PROJECT_SOCKET_H
