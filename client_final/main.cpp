#include <iostream>
#include "con_handler.h"

int main(int argc, char** argv) {
    Out out;

    try{
        if(argc!=3){
            throw std::runtime_error("Passare come parametri al programma indirizzo ip del server e porta destinazione.");
        }
        boost::asio::io_context io_context;
        con_handler c(io_context, argv[1], argv[2]);
        c.start(io_context);
    } catch (boost::system::system_error& error){ //errori in stabilire connessione -> termina programma
        if(error.code().value()==ECONNREFUSED || error.code().value()==ECONNABORTED || error.code().value()==ECONNRESET
        //|| error.code().value()==WSAECONNREFUSED || error.code().value()==WSAECONNABORTED || error.code().value()==WSAECONNRESET //da commentare se non si è in Windows
        ) {
            out("Errore di connessione: connessione rifiutata.");
            return -1;
        } else {
            out(error.what());
            return -1;
        }
    } catch (std::exception& e) {
        out(e.what());
        return -1;
    }

    return 0;
}
