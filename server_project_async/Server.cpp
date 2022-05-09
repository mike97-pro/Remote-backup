//
// Created by mike_ on 28/07/2020.
//

#include "Server.h"
#include <boost/optional.hpp>
#include <boost/utility/typed_in_place_factory.hpp>
#include <boost/filesystem.hpp>

void Server::start_accept() {
    // socket
    con_handler::pointer connection = con_handler::create(io_service_);

    // asynchronous accept operation and wait for a new connection.
    acceptor_.async_accept(connection->socket(),
                           boost::bind(&Server::handle_accept, this, connection,
                                       boost::asio::placeholders::error));
}

void Server::handle_accept(con_handler::pointer connection, const boost::system::error_code &err) {
    if (!err) {
        //connection->start();
        log->write_log("New connection by "+connection->socket().remote_endpoint().address().to_string());
        connQueue.put(connection);
    }
    start_accept();
}

void Server::stop()
{
    // The server is stopped by cancelling all outstanding asynchronous
    // operations. Once all operations have finished the io_context::run()
    // call will exit.
    acceptor_.close();
    io_service_.stop();
    connQueue.finish();
    for(int k=0; k<Nthreads; k++)
        threads[k].detach();

    if(reset){
        printer("Reset del server in corso...");
        usersDB->reset_DB();
        auto p = boost::filesystem::directory_iterator(PATH);
        for(p; p!=boost::filesystem::directory_iterator(); p++){
            boost::filesystem::remove_all(p->path().c_str());
        }
    }
    
    printer("\nServer is closed.");
    log->write_log("\nServer stopped on ");
}

void Server::work() {
    std::stringstream s;
    s<<std::thread::id(std::this_thread::get_id());
    while (!connQueue.ended()){
        auto conn = connQueue.get();
        if(conn.has_value()){
            log->write_log("Host "+conn.value()->socket().remote_endpoint().address().to_string()+" served by thread "+s.str());
            conn.value()->start();
        }
    }
    log->write_log("Thread "+s.str()+" done ");
}

void Server::control_panel() {
    std::string scelta;
    while(scelta!="0"){
        printer("\tPannello di Controllo\n0-> Chiudi Server\n1-> Pulisci file di log\n2-> Reset Server");
        std::getline(std::cin,scelta);
        if(scelta=="1"){
            log->clean();
            printer("Log pulito.\n");
        }
        else if (scelta == "2"){
            std::string bin;
            while(bin!="S" && bin!="N") {
                printer("Il reset del server comportera' la cancellazione di tutti gli utenti e dei backup ad essi associati. Continuare? S/N");
                std::getline(std::cin,bin);
                if(bin=="S") {
                    reset = true;
                    printer("Il server verra' resettato alla chiusura.\n");
                }
                else if(bin!="N")
                    printer("Opzione non valida");
            }
        }
        else if(scelta=="0")
            stop();
        else
            printer("Opzione non valida.\n");
    }
}

void Server::wait_cp() {
    cp_status.wait();
}





