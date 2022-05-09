//
// Created by mentz on 19/08/20.
//

#include "con_handler.h"
#define MENU "\n\t\tMENU\n0-> Log out\n1-> Inizia sincronizzazione\n2-> Specifica path da sincronizzare\n3-> Visualizza backup su server\n4-> Elimina un backup\n5-> Ripristina un backup\n6-> Opzioni utente"

using work_guard_type = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

bool reconnect(boost::shared_ptr<Socket>& soc, std::string& email, std::string& pw, std::shared_ptr<Dir> root,
        std::shared_ptr<Jobs<Message>> messagges, std::atomic<bool>& running, std::atomic<bool>& con_problems);

void import_backup(std::string& answer, boost::shared_ptr<Socket>& soc);

con_handler::con_handler(boost::asio::io_context& io_context, std::string name, std::string port):
soc(Socket::create(io_context)), messagges(new Jobs<Message> (20)), name(name) {
    this->port=std::stoi(port);
    soc->connect(this->name, this->port);
    con_status=CONNECTED;
}

void con_handler::start(boost::asio::io_context& io_context){
    int retry;
    std::string opt, answer, email, pw, path;
    std::future<void> asy, f;
    //variabile atomica per far terminare tutti i thread correttamente
    std::atomic<bool> running(true), con_problems(false);
    bool in_synch=false;
    Out out;

    try{ //tenta una prima read per verificare la connessione
        opt=soc->read();
    } catch (boost::system::system_error& error) {
        std::cout<<error.what()<<" --- ";
        throw std::runtime_error("Host unreachable");
    }

    out("\t\t" + opt);

    while(con_status!=CLOSE) { //errori non in fase di sincornizzazione causano la chiusura del programma
        switch (con_status) {
            case CONNECTED: {
                try {
                    connected();
                } catch (boost::system::system_error& error) {
                    out(std::string(error.what()) + " --- ");
                    throw std::runtime_error("Host unreachable");
                }
                break;
            }
            case LOG_IN: {
                try {
                    log_in(retry, answer, email, pw, path);
                } catch (boost::system::system_error& error) {
                    out(std::string(error.what()) + " --- ");
                    throw std::runtime_error("Host unreachable");
                }
                break;
            }
            case SIGN_IN: {
                try {
                    sign_in(retry, answer, email, pw);
                } catch (boost::system::system_error& error) {
                    out(std::string(error.what()) + " --- ");
                    throw std::runtime_error("Host unreachable");
                }
                break;
            }
            case LOGGED: {
                try {
                    logged(asy, f, answer, io_context, in_synch, running, retry,
                            con_problems, email, pw, path);
                } catch(boost::filesystem::filesystem_error& err){
                    out("Errore: path non valido, provare a inserire un altro path.");
                    out(MENU);
                }catch (boost::system::system_error& error) {
                    out(std::string(error.what()) + " --- ");
                    throw std::runtime_error("Host unreachable");
                }
                break;
            }
            case CLOSE:
                break;
        }
    }
}

void con_handler::connected(){
    Out out;
    bool invalid = false;
    out("0-> Exit\n1-> Log in\n2-> Sign in");
    std::string decision;
    std::getline(std::cin, decision);

    if (decision == "1") {
        con_status = LOG_IN;
    } else if (decision == "2") {
        con_status = SIGN_IN;
    } else if (decision == "0") {
        con_status = CLOSE;
    } else {
        out("Opzione non valida.");
        invalid = true;
    }
    if (!invalid)
        soc->write(decision);
}

void con_handler::log_in(int& retry, std::string& answer, std::string& email, std::string& pw, std::string& path) {
    Out out;
    bool go_back=false;
    retry = 1;
    out("\n\t\tLOG IN");
    while (retry) {
        out("Inserisci email: ");
        std::getline(std::cin, email);
        soc->write(email);
        answer = soc->read();
        if (answer == "not_registered") {
            out("Utente non registrato.\n0-> Riprova\nQualsiasi altro tasto-> Indietro");
            std::getline(std::cin, answer);
            if (answer != "0") {
                con_status = CONNECTED;
                go_back = true;
                retry = 0;
                out("\n");
            }
            soc->write(answer);
        } else {
            retry = 0;
        }
    }

    if(!go_back) {
        retry = 3;
        while (retry) {
            try {
                out("Inserisci password per " + email + ": ");
                std::getline(std::cin, pw);
                soc->write(sha256(pw));
                answer = soc->read();
                if (answer == "logged") {
                    con_status = LOGGED;
                    //user = email;
                    soc->write("ok");
                    answer = soc->read();
                    out("\n\t\t" + answer);
                    answer = soc->read();
                    if(answer == "path_avaiable"){
                        path = soc->read();
                    }
                    out(MENU);
                    break;
                } else {
                    throw std::runtime_error("Fail: invalid password");
                }
            } catch (std::runtime_error &error) {
                retry--;
                if(!retry)
                    throw error;
                out(error.what());
                out("0-> Riprova (" + std::to_string(retry) + " tentativi rimanenti)\n"
                                                              "Qualsiasi altro tasto-> Indietro");
                std::getline(std::cin, answer);
                soc->write(answer);
                if (answer != "0") {
                    retry = 0;
                    con_status = CONNECTED;
                    out("");
                }
            }
        }
    }
}

void con_handler::sign_in(int& retry, std::string& answer, std::string& email, std::string& pw) {
    Out out;
    bool go_back = false;
    retry = 1;
    std::string c_pw, nick;
    std::cout << "\n\t\tSIGN IN" << std::endl;

    while (retry) {
        out("Insert email: ");
        std::getline(std::cin, email);
        soc->write(email);
        answer = soc->read();
        if (answer == "already_in") {
            out("Utente gia' registrato.\n0-> Riprova\nQualsiasi altro tasto-> Indietro");
            std::getline(std::cin, answer);
            if (answer != "0") {
                con_status = CONNECTED;
                go_back = true;
                retry = 0;
                out("");
            }
            soc->write(answer);
        } else {
            retry = 0;
            out("Inserisci nickname: ");
            std::getline(std::cin, nick);
            soc->write(nick);
        }
    }

    if(!go_back) {
        retry=1;
        while (retry) {
            out("Inserisci password: ");
            std::getline(std::cin, pw);
            out("Conferma password: ");
            std::getline(std::cin, c_pw);
            if (pw != c_pw) {
                out("Password diverse! Riprova.");
            } else {
                retry = 0;
                soc->write(sha256(pw));
                con_status = LOG_IN;
            }
        }
    }
}

void con_handler::logged(std::future<void>& asy, std::future<void>& f, std::string& answer,
        boost::asio::io_context& io_context, bool& in_synch, std::atomic<bool>& running, int& retry,
        std::atomic<bool>& con_problems, std::string& email, std::string& pw, std::string& path){

    Out out;
    bool go_back=false;
    std::string choice, pth;

    if(in_synch){ //se ero già in sincronizzazione
        out("Premere 0 per tornare al menu.");
        std::getline(std::cin, answer);
        if (answer == "0") { //altrimenti ignora
            if(con_problems.load()){ //stoppato mentre c'erano problemi di connessione
                running.store(false);
                io_context.stop();
                out("Il programma sta per essere terminato.");
                f.get();
                asy.get();
                in_synch = false;
                con_status = CLOSE;
            } else {
                running.store(false);
                soc->write(answer);
                f.get();
                asy.get();
                in_synch = false;
                con_status = LOGGED;
                out(MENU);
            }
        }
    } else {
        std::getline(std::cin, answer);
        if (answer=="0") {
            con_status = CONNECTED;
            path.clear();
            soc->write(answer);
            out("");
        } else if(answer=="1") {
            if (!fs::exists(fs::path(path))) {
                out("Nessun path specificato! Inserire path da sincronizzare");
                retry = 1;
            } else
                retry = 0;

            while (retry) {
                std::getline(std::cin, pth);
                if (!fs::exists(fs::path(pth))) {
                    out("Path inesistente.");
                    out("0-> Riprova\nQualsiasi altro tasto-> Indietro");
                    std::getline(std::cin, pth);
                    if (pth != "0") {
                        go_back = true;
                        retry = 0;
                        out(MENU);
                    } else {
                        out("Inserire path valido");
                    }
                } else if (!fs::is_directory(fs::path(pth))) {
                    out("Il path specificato non rappresenta una directory.");
                    out("0-> Riprova\nQualsiasi altro tasto-> Indietro");
                    std::getline(std::cin, pth);
                    if (pth != "0") {
                        go_back = true;
                        retry = 0;
                        out(MENU);
                    } else {
                        out("Inserire path valido.");
                    }
                } else {
                    choice="2";
                    soc->write(choice);
                    path=Dir::normalizza_path(pth);
                    soc->write(path);
                    choice = soc->read();
                    if(choice=="ok")
                        retry = 0;
                    else {
                        out("Errore sul server.");
                        path.clear();
                        out("0-> Riprova\nQualsiasi altro tasto-> Indietro");
                        std::getline(std::cin, pth);
                        if (pth != "0") {
                            retry = 0;
                            go_back= true;
                            out(MENU);
                        }
                    }
                }
            }

            //path specificato ed esistente -> inizia sincronizzazione
            if (!go_back) {
                //mette a posto variabili prima di far partire il wathcer vero e proprio
                running.store(true);
                messagges->restart();
                soc->write(answer);
                fs::path pi;

                try {
                    if (!fs::path(path).is_absolute())
                        pi = fs::canonical(path);
                    else
                        pi = path;
                } catch (fs::filesystem_error &error) { //fs::canonical non è supportato da tutti i compilatori
                    pi = path;
                }

                pi = Dir::normalizza_path(pi.string());

                soc->write(pi.string());

                f = (std::async(std::launch::async, [&io_context, &running, &con_problems, this, &email, &pw]() {
                    while (running.load()) {
                        Out out;
                        try {
                            if (con_problems.load()) { //se ci sono stati errori di connessione
                                this->soc->connect(this->name, this->port);
                                // se suupera connect senza lanciare eccezione si può ricollegare e il tutto può ripartire
                                if(!reconnect(this->soc, email, pw, this->root, this->messagges, running, con_problems)) {
                                    out("Riconnesione non riuscita! Premere 0 e terminare il programma.");
                                    return;
                                }
                            }
                            if (io_context.stopped())
                                io_context.restart();
                            if (running.load()) { //potrebbe essere cambiato mentre si è riconnesso
                                work_guard_type work_guard(io_context.get_executor());
                                io_context.run();
                            }
                            break;
                        } catch (boost::system::system_error &error) {
                            if (error.code().value() == ECONNREFUSED
                                //|| error.code().value() == WSAECONNREFUSED //da commentare se non si è in Windows
                                    ) {
                                std::this_thread::sleep_for(std::chrono::seconds(3));
                            } else {
                                io_context.stop();
                                con_problems = true;
                                out(std::string(error.what()) + " --- Connessione con il "
                                                                "server persa\nTentando la riconnessione... -- "
                                                                "Premere 0 per terminare il programma.");
                            }
                        }
                    }
                }));


                in_synch = true;

                asy = std::async(std::launch::async, [&running, this, pi, &con_problems]() {
                    con_handler::start_sync(pi.string(), running, this->soc, this->messagges, con_problems,
                                            this->root);
                });

            }
        } else if(answer=="2") {
            retry = 1;

            if(!path.empty()) {
                out("Ultimo path selezionato: " + path);
                out("Cambiare path? S/N");
                std::getline(std::cin, pth);
                if(pth!="S" && pth!="s")
                    retry=0;
            }

            while (retry) {
                out("Inserire path da sincronizzare");
                std::getline(std::cin, pth);
                if (!fs::exists(fs::path(pth))) {
                    out("Path inesistente: inserire path valido.");
                    out("0-> Riprova\nQualsiasi altro tasto-> Indietro");
                    std::getline(std::cin, pth);
                    if (pth != "0") {
                        retry = 0;
                    }
                } else { //mette a posto variabili prima di far partire il wathcer vero e proprio
                    retry=0;
                    soc->write(answer);
                    path = Dir::normalizza_path(pth);
                    soc->write(path);
                    choice=soc->read();
                    if(choice=="ok")
                        retry = 0;
                    else {
                        out("Errore sul server.");
                        path.clear();
                        out("0-> Riprova\nQualsiasi altro tasto-> Indietro");
                        std::getline(std::cin, pth);
                        if (pth != "0") {
                            retry = 0;
                        }
                        else
                            soc->write(answer);
                    }
                }
            }

            out(MENU);
        } else if(answer=="3") {
            soc->write(answer);
            out("");
            auto n = std::stoi(soc->read());
            if (n == 0) {
                out("Nessun backup presente sul server.");
            } else {
                for (int i = 0; i < n; i++) {
                    out(soc->read());
                }
            }
            out("");
            out("Premere qualsiasi tasto per tornare al menu.");
            std::getline(std::cin, answer);
            out(MENU);
        } else if(answer=="4") {
            out("Inserisci il path assoluto del backup da eliminare.");
            std::getline(std::cin, pth);
            if(!fs::path(pth).is_absolute()){
                out("Inserire il path assoluto.");
            } else {
                soc->write(answer);
                pth = Dir::normalizza_path(pth);
                soc->write(pth);
                answer = soc->read();
                if (answer == "0") {
                    out("Path non valido.");
                } else {
                    out("Backup eliminato correttamente");
                }
            }
            out("");
            out("Premere qualsiasi tasto per tornare al menu.");
            std::getline(std::cin, answer);
            out(MENU);
        } else if(answer=="5") {
            out("Operazione sincrona non interrompibile. Continuare? S/N");
            std::getline(std::cin, choice);
            if(choice=="S" || choice=="s") {
                out("Inserisci il path della directory da ripristinare.");
                std::getline(std::cin, pth);
                if(!fs::path(pth).is_absolute()){
                    out("Inserire il path assoluto.");
                } else {
                    soc->write(answer);
                    pth = Dir::normalizza_path(pth);
                    soc->write(pth);
                    if (soc->read() == "0") {
                        out("Path non valido.");
                    } else {
                        import_backup(pth, soc);
                        out("Backup ripristinato con successo");
                    }
                }
                out("");
                out("Premere qualsiasi tasto per tornare al menu.");
                std::getline(std::cin, answer);
            }
            out(MENU);
        } else if(answer=="6"){
            soc->write(answer);
            std::string ans;
            while(choice!="0"){
                out("\n1-> Cambio email\n2-> Cambio nick-name\n3-> Cambio password\n0-> Torna al menu");
                std::getline(std::cin,choice);
                soc->write(choice);
                if(choice =="1"){
                    std::string new_email="0";
                    while(new_email=="0") {
                        out("Inserisci nuova mail: ");
                        std::getline(std::cin, new_email);
                        if (new_email == email) {
                            out("L'email non puo' essere uguale a quella vecchia!\n0-> Riprova\nQualsiasi altro tasto-> Indietro");
                            std::getline(std::cin, new_email);
                            if(new_email!="0")
                                soc->write("undo");
                        }
                        else{
                            soc->write(new_email);
                            ans=soc->read();
                            if(ans == "already_in"){
                                out("L'email e' gia' in uso da un altro utente\n0-> Riprova\nQualsiasi altro tasto-> Indietro");
                                std::getline(std::cin, new_email);
                                soc->write(new_email);
                            }
                            else{
                                email = new_email;
                                out("Email sostituita con successo");
                            }
                        }
                    }
                }
                else if(choice == "2"){
                    std::string new_nick="0";
                    while(new_nick=="0"){
                        out("Inserisci nuovo nickname: ");
                        std::getline(std::cin,new_nick);
                        soc->write(new_nick);
                        ans = soc->read();
                        if(ans == "error"){
                            out("Errore durante la sostituzione\n0-> Riprova\nQualsiasi altro tasto-> Indietro");
                            std::getline(std::cin, new_nick);
                            soc->write(new_nick);
                        }
                        else{
                            out("Nickname sostituito con successo");
                        }
                    }
                }
                else if(choice=="3"){
                    std::string new_pw="0";
                    while(new_pw=="0"){
                        out("Inserisci vecchia password: ");
                        std::getline(std::cin, new_pw);
                        if(new_pw!=pw){
                            out("Password errata\n0-> Riprova\nQualsiasi altro tasto-> Indietro");
                            std::getline(std::cin, new_pw);
                            if(new_pw!="0")
                                soc->write("undo");
                        }
                        else{
                            std::string c_pw;
                            out("Inserisci nuova password: ");
                            std::getline(std::cin, new_pw);
                            out("Conferma nuova password: ");
                            std::getline(std::cin, c_pw);
                            if(new_pw != c_pw){
                                out("Le password non corrispondono!\n0-> Riprova\nQualsiasi altro tasto-> Indietro");
                                std::getline(std::cin, new_pw);
                                if(new_pw!="0")
                                    soc->write("undo");
                            }
                            else{
                                soc->write(sha256(new_pw));
                                ans=soc->read();
                                if(ans == "error"){
                                    out("Errore durante la sostituzione\n0-> Retry\nAny other key-> Go back");
                                    std::getline(std::cin, new_pw);
                                    soc->write(new_pw);
                                }
                                else{
                                    pw = new_pw;
                                    out("Password sostituita con successo");
                                }
                            }
                        }
                    }
                }
                else if(choice != "0")
                    out("Opzione non valida");
            }
            out(MENU);
        } else {
            out("Opzione non valida.");
            out(MENU);
        }
    }
}

void con_handler::start_sync(std::string s, std::atomic<bool>& running, boost::shared_ptr<Socket>& soc,
                             std::shared_ptr<Jobs<Message>> messagges, std::atomic<bool>& con_problems,
                             std::shared_ptr<Dir>& root){

    Out out;

    soc->setMes(messagges);
    soc->async_read_command(); //ciclo infinito di letture asincrone finchè non riceve 0

    auto p = fs::path(s);

    try {
        //crea immagine path in oggetto root di classe Dir
        root = Dir::createRoot(p, running);
    } catch (fs::filesystem_error& err) {
        out("Errore sul path selezionato ---" + err.path1().string()+ "\nScegliere un altro path.");
        out("Premere 0 per tornare al menu e selezionare un altro path.");
        return;
    } catch (std::exception& e) {
        out(e.what());
        out("Premere 0 per tornare al menu e selezionare un altro path.");
        return;
    }

    std::thread consumer([&messagges, &soc, &con_problems, &running](){
        while (true) {
            std::optional<Message> m = messagges->get();
            if (!m.has_value())
                break;
            if(!con_problems.load())
                m->consume(soc, running);
        }
    });

    //Host verifica se il server ha già le cartelle sincronizzate e in caso contrario le sincronizza
    Message mstart=Message::startVerify(root->getName());
    messagges->put(mstart);
    root->verify_sinch(messagges, running);
    Message mend=Message::endVerify();
    messagges->put(mend);

    //File watcher che monitora modifiche in path
    FileWatcher fw{p.string(), std::chrono::milliseconds(500)};

    while(running) { //overload di errori su file temporanei
        try {
            fw.start(action, running, root, messagges);
        } catch (fs::filesystem_error &error) {
            if(error.path1()!=root->getName())
                continue;
            out("Errore --- "+std::string(error.what())+" --- Path non più valido.");
            out("Premere 0 per tornare al menu e selezionare un altro path.");
            running.store(false);
        } catch (std::exception &e) {
            std::cout << e.what() << std::endl;
            running.store(false);
            out("Premere 0 per tornare al menu.");
        }
    }

    messagges->finish();
    consumer.join();
}

con_handler::~con_handler() {
    soc->close();
}

bool reconnect(boost::shared_ptr<Socket>& soc, std::string& email, std::string& pw, std::shared_ptr<Dir> root,
        std::shared_ptr<Jobs<Message>> messagges, std::atomic<bool>& running, std::atomic<bool>& con_problems){
    Out out;
    soc->read();
    std::string decision("1");
    soc->write(decision);
    soc->write(email);
    auto r=soc->read();
    if(r=="not_registered")
        return false;
    soc->write(sha256(pw));
    soc->read();
    decision="ok";
    soc->write(decision);
    soc->read();
    soc->read();
    soc->read();
    decision="1";
    soc->write(decision);
    soc->write(root->getName());
    out("Riconnessione riuscita!");
    con_problems = false;
    soc->async_read_command();
    Message mstart=Message::startVerify(root->getName());
    messagges->put(mstart);
    root->verify_sinch(messagges, running);
    Message mend=Message::endVerify();
    messagges->put(mend);
    return true;
}

void import_backup(std::string& answer, boost::shared_ptr<Socket>& soc){
    if(fs::exists(answer))
        fs::remove_all(answer);
    while(true){
        auto mess=soc->read();
        if(mess=="0")
            break;
        std::stringstream buf;
        buf.str(mess);
        boost::property_tree::ptree pt;
        boost::property_tree::read_json(buf, pt);
        auto pth=pt.get<std::string>("path");
        fs::path rel(answer);
        rel=rel.parent_path();
        rel/=pth;
        pth=rel.string();
        auto type=pt.get<std::string>("type");
        if(type=="0"){
            fs::create_directories(pth);
        } else {
            soc->read_file(pt, pth);
        }
    }
}