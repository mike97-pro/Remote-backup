//
// Created by mike_ on 28/07/2020.
//

#ifndef PDS_PROJECT_2020_SERVER_SERVER_H
#define PDS_PROJECT_2020_SERVER_SERVER_H
#include "con_handler.h"
#include "Queue.h"
#include <thread>
#include <vector>
#include <future>

class Server
{
private:
    tcp::acceptor acceptor_;
    boost::asio::io_service& io_service_;
    Queue<con_handler::pointer> connQueue;
    int Nthreads;
    std::vector<std::thread> threads;
    mutex_print printer;
    std::shared_ptr<Logger> log;
    std::shared_ptr<UsersDB> usersDB;
    std::future<void> cp_status;
    bool reset = false;

    void work();
    void start_accept();
    void handle_accept(con_handler::pointer connection, const boost::system::error_code& err);
    void stop();
    void control_panel();
public:
//constructor for accepting connection from client
    Server(boost::asio::io_service& io_service, int maxConn, int nThreads, uint16_t port_num): Nthreads(nThreads), connQueue(maxConn), io_service_(io_service), acceptor_(io_service, tcp::endpoint(tcp::v4(), port_num)){
        if(nThreads<1 || maxConn<1)
            throw std::logic_error("value for nThreads/maxConn must be >0");
        for(int k=0; k<Nthreads; k++)
            threads.emplace_back([this](){work();});

        usersDB = UsersDB::get_DB();
        log = Logger::getLogger();

        ip::tcp::resolver resolver(io_service);//ottengo gli indirizzi ip locali utilizzando il nome del server stesso per la risoluzione
        printer("\nHost name is "+ip::host_name());
        printer("IPv4 addresses are: ");
        std::for_each(resolver.resolve({ip::host_name(), ""}), {}, [this](const auto& re) {
            if(re.endpoint().address().is_v4())
                printer(re.endpoint().address().to_string());
        });

        printer("\nServer is listening on "+acceptor_.local_endpoint().address().to_string()+" port "+std::to_string(acceptor_.local_endpoint().port()));

        cp_status = std::async(std::launch::async,[this](){control_panel();});

        start_accept();
    }

    void wait_cp();


};


#endif //PDS_PROJECT_2020_SERVER_SERVER_H
