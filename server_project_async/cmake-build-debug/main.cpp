#include <iostream>
#include "Server.h"

int main(int argc, char *argv[])
{
    if(argc != 2){
        std::cout<<"Parametri accettati: numero di porta"<<std::endl;
        return -1;
    }
    int port_number = atoi(argv[1]);
    if(port_number<1024 || port_number>65535){
        std::cout<<"Numero di porta non valido. Inserire un valore compreso tra 1024 e 65535"<<std::endl;
        return -1;
    }
    try
    {
        boost::asio::io_service io_service;
        Server server(io_service,1000,3,port_number);
        io_service.run();
        server.wait_cp();
    }
    catch(std::exception& e)
    {
        std::cerr << e.what() << endl;
    }
    return 0;
}