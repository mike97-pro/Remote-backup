//
// Created by mike_ on 29/07/2020.
//

#include "con_handler.h"
#include <ctime>
#include <string>
#include <boost/array.hpp>
#include <boost/filesystem.hpp>
#include <vector>
#include "fs/Dir.h"
#include <algorithm>

con_handler::pointer con_handler::create(boost::asio::io_service& io_service){
    return pointer(new con_handler(io_service));
}

tcp::socket& con_handler::socket(){
    return sock;
}

std::string read_f(tcp::socket& socket){
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

void write_f(tcp::socket& socket, const std::string& buffer){
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

void read_file(tcp::socket& socket, boost::property_tree::ptree pt, std::string abs_path, Queue<std::vector<char>>& commands){
    auto size=pt.get<size_t>("size");
    int num_pac = size / 1024;
    std::ofstream f(abs_path, std::ios::binary);
    if(size!=0) {
        for (int i = 0; i <= num_pac; i++) {
            auto v=commands.get();
            if(!v.has_value())
                break;
            f.write(v->data(), v->size()-1);
        }
    }
    f.close();
}

void write_file(tcp::socket& sock, const std::string& p){
    size_t size=boost::filesystem::file_size(p);
    size_t read=0;
    std::ifstream f(p, std::ios::binary);
    size_t to_read;
    boost::array<char, 1024> buf;
    boost::system::error_code error;
    while(read<size) {
        to_read=size-read;
        if(to_read>1024)
            to_read=1024;
        f.read(buf.data(), to_read);
        read+=to_read;
        boost::asio::write(sock, boost::asio::buffer(&to_read, sizeof(to_read)), error);
        if(error) {
            throw boost::system::system_error(error);
        }
        boost::asio::write(sock, boost::asio::buffer(buf, to_read), error);
        if(error) {
            throw boost::system::system_error(error);
        }
    }
    f.close();
}

void con_handler::async_write(tcp::socket& socket, const std::string& buffer) {
    std::size_t size=buffer.size();
    boost::asio::async_write(socket, boost::asio::buffer(&size, sizeof(size_t)),
                             boost::bind(&con_handler::handle_write,
                                         shared_from_this(),
                                         boost::asio::placeholders::error,
                                         boost::asio::placeholders::bytes_transferred));
    boost::asio::async_write(socket,
                             boost::asio::buffer(buffer, size),
                             boost::bind(&con_handler::handle_write,
                                         shared_from_this(),
                                         boost::asio::placeholders::error,
                                         boost::asio::placeholders::bytes_transferred));
}

void con_handler::async_read_command(){
    boost::asio::async_read(sock,
            boost::asio::buffer((char*)&sizeOfdata, sizeof(size_t)),
            boost::bind(&con_handler::handle_read_command_size,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
}

void con_handler::handle_read_command_size(const boost::system::error_code &err, size_t bytes_transferred) {
    if (!err) {
        data.resize(sizeOfdata);
        boost::asio::async_read(sock,
                                boost::asio::buffer(data, sizeOfdata),
                                boost::bind(&con_handler::handle_read_command,
                                            shared_from_this(),
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    } else {
        std::cerr << "error: " << err.message() << std::endl;
        sock.close();
        commands.finish();
    }
}

void con_handler::handle_read_command(const boost::system::error_code &err, size_t bytes_transferred) {
    if (!err) {
        data.push_back('\0');
        if(data[0]=='0' && data.size()==2){
            commands.finish();
            std::string d(data.data());
            write_f(sock, d);
        }
        else {
            commands.put(data);
            async_read_command();
        }
    } else {
        std::cerr << "error: " << err.message() << std::endl;
        sock.close();
        commands.finish();
    }
}

void con_handler::start() {
    std::string messages;
    std::string ip = sock.remote_endpoint().address().to_string();
    messages="\t\tWELCOME";
    write_f(sock,messages);
    while (con_status != CLOSE) {
        switch (con_status) {
            case CONNECTED: {
                try {
                    auto answer = read_f(sock);

                    if (answer == "1") {
                        con_status = LOG_IN;
                    } else if (answer == "2") {
                        con_status = SIGN_IN;
                    } else if (answer == "0") {
                        con_status = CLOSE;
                    }
                } catch (boost::system::system_error& error) {
                    con_status=CLOSE;
                }
                break;
            }
            case LOG_IN: {
                try {
                    auto email = read_f(sock);
                    auto u = usersDB->get_user(email);
                    if (u.has_value()) {
                        messages = "ok";
                        write_f(sock, messages);
                        bool retry = true;
                        while (retry) {
                            auto pw = read_f(sock);
                            if (pw == u->get_password()) {
                                owner = u.value();
                                con_status = LOGGED;
                                messages = "logged";
                                write_f(sock, messages);
                                auto answer = read_f(sock);
                                messages = "Welcome " + owner.get_nickname();
                                write_f(sock, messages);
                                log->write_log("Log in by "+owner.get_email()+" ip_addr "+ip);
                                if(!owner.get_path().empty()){
                                    write_f(sock,"path_avaiable");
                                    write_f(sock,owner.get_path());
                                }
                                else
                                    write_f(sock,"no_path");
                                break;
                            } else {
                                messages = "err";
                                write_f(sock, messages);
                                auto answer = read_f(sock);
                                if (answer != "0") {
                                    retry = false;
                                    con_status = CONNECTED;
                                }
                            }
                        }
                    } else {
                        messages = "not_registered";
                        write_f(sock, messages);
                        auto answer = read_f(sock);
                        if (answer != "0")
                            con_status = CONNECTED;
                    }
                    break;
                } catch (boost::system::system_error &error) {
                    con_status = CLOSE;
                }
            }
            case SIGN_IN: {
                try {
                    auto email = read_f(sock);
                    auto u = usersDB->get_user(email);
                    if (!u.has_value()) {
                        messages = "ok";
                        write_f(sock, messages);
                        auto nick = read_f(sock);
                        std::string pw = read_f(sock);
                        usersDB->insert_user(User(email, nick, pw));
                        con_status = LOG_IN;
                    } else {
                        messages = "already_in";
                        write_f(sock, messages);
                        auto answer = read_f(sock);
                        if (answer != "0")
                            con_status = CONNECTED;
                    }
                    break;
                } catch (boost::system::system_error &err) {
                    con_status = CLOSE;
                }
            }
            case LOGGED: {
                try {
                    auto answer = read_f(sock);
                    if (answer == "0") {
                        log->write_log("Log out by "+owner.get_email()+" ip_addr "+ip);
                        if(!commands.empty())
                            finish_syncing();
                        con_status = CONNECTED;
                    } else if (answer == "1") {
                        sync();
                    } else if(answer == "2"){
                        auto pth = read_f(sock);
                        if(usersDB->set_path(owner.get_email(),pth)) {
                            owner.set_path(pth);
                            write_f(sock,"ok");
                        }
                        else
                            write_f(sock,"some_error");
                    } else if (answer=="3"){
                        boost::filesystem::path s_path(PATH);
                        s_path/=owner.get_email();
                        int n=0;
                        if(fs::exists(s_path)) {
                            for (auto &i:fs::directory_iterator(s_path)) {
                                n++;
                            }
                            write_f(sock, std::to_string(n));
                            for (auto &i:fs::directory_iterator(s_path)) {
                                std::string s = i.path().filename().string();
                                std::replace(s.begin(), s.end(), '-', '/');
                                std::replace(s.begin(), s.end(), '&', ':');
                                write_f(sock, s);
                            }
                        } else {
                            write_f(sock, std::to_string(n));
                        }
                    } else if (answer=="4"){
                        answer=read_f(sock);
                        boost::filesystem::path s_path(PATH);
                        s_path/=owner.get_email();
                        std::replace(answer.begin(), answer.end(), '/', '-');
                        std::replace(answer.begin(), answer.end(), ':', '&');
                        s_path/=answer;
                        std::string server_path=s_path.string();
                        if(fs::exists(server_path)){
                            fs::remove_all(server_path);
                            answer="1";
                        } else {
                            answer="0";
                        }
                        write_f(sock, answer);
                    } else if (answer=="5") {
                        answer = read_f(sock);
                        boost::filesystem::path s_path(PATH);
                        s_path /= owner.get_email();
                        std::replace(answer.begin(), answer.end(), '/', '-');
                        std::replace(answer.begin(), answer.end(), ':', '&');
                        s_path /= answer;
                        std::string server_path = s_path.string();
                        if (fs::exists(server_path)) {
                            answer = "1";
                            write_f(sock, answer);
                            send_backup(server_path);
                        }
                        answer = "0";
                        write_f(sock, answer);
                    }
                    else if(answer=="6"){
                        std::string scelta;
                        while(scelta!="0"){
                            scelta = read_f(sock);
                            if(scelta =="1"){
                                std::string new_email="0";
                                while(new_email == "0") {
                                    new_email=read_f(sock);
                                    if (new_email != "undo") {
                                        if (usersDB->change_email(owner.get_email(), new_email) == -1) {
                                            write_f(sock, "already_in");
                                            new_email = read_f(sock);
                                        }
                                        else {
                                            std::string prfx = PATH;
                                            rename(std::string(prfx+"/"+owner.get_email()).c_str(),std::string(prfx+"/"+new_email).c_str());
                                            owner.set_email(new_email);
                                            write_f(sock, "ok");
                                        }
                                    }
                                }
                            }
                            else if(scelta == "2"){
                                std::string new_nick="0";
                                while(new_nick=="0"){
                                    new_nick=read_f(sock);
                                    if(usersDB->change_nick(owner.get_email(),new_nick) == -1){
                                        write_f(sock,"error");
                                        new_nick = read_f(sock);
                                    }
                                    else{
                                        owner.set_nickname(new_nick);
                                        write_f(sock,"ok");
                                    }
                                }
                            }
                            else if(scelta=="3"){
                                std::string new_pw="0";
                                while(new_pw=="0"){
                                    new_pw=read_f(sock);
                                    if(new_pw!="undo"){
                                        if (usersDB->change_password(owner.get_email(), new_pw) == -1) {
                                            write_f(sock, "error");
                                            new_pw = read_f(sock);
                                        }
                                        else {
                                            owner.set_password(new_pw);
                                            write_f(sock, "ok");
                                        }
                                    }
                                }
                            }
                        }
                    }
                    break;
                } catch (boost::system::system_error& err){
                    log->write_log("Log out by "+owner.get_email()+" ip_addr "+ip);
                    if(!commands.empty())
                        finish_syncing();
                    con_status=CLOSE;
                }catch (std::exception& err){
                    log->write_log("Log out by "+owner.get_email()+" ip_addr "+ip);
                    if(!commands.empty())
                        finish_syncing();
                    con_status=CLOSE;
                }
            }
        }
    }
}

void con_handler::sync(){
    auto pth=read_f(sock);

    boost::property_tree::ptree pt;
    std::stringstream buf;
    std::shared_ptr<Dir> root;
    boost::filesystem::path s_path(PATH);
    s_path/=owner.get_email();
    std::replace(pth.begin(), pth.end(), '/', '-');
    std::replace(pth.begin(), pth.end(), ':', '&');

    s_path/=pth;
    std::string server_path=s_path.string();
    std::vector<char> str;
    commands.start();
    async_read_command();
    while(!commands.ended())
    {
        boost::property_tree::ptree pt_write;
        auto command = commands.get();
        if(command.has_value()) {
            str= command.value();
        }
        else {
            break;
        }

        buf.str(str.data());
        boost::property_tree::read_json(buf, pt);
        auto action=pt.get<std::string>("action");
        auto type=pt.get<int>("type");
        auto rel_path=pt.get<std::string>("path");
        fs::path apath(server_path);
        apath/=rel_path;
        auto abs_path=Dir::normalizza_path(apath.string());
        if(action=="S"){
            if(!fs::exists(abs_path)){
                fs::create_directories(abs_path);
            }
            root=Dir::createRoot(abs_path);
        } else if(action=="V"){
            if(type==0){ //directory
                if(fs::exists(abs_path)){
                    auto last_mod=pt.get<time_t>("last_mod");
                    auto a (root->getDir(abs_path));
                    a->setlast_mod(last_mod);
                    pt_write.put("outcome", "1");
                    pt_write.put("path", rel_path);
                    pt_write.put("last_sync", a->getlast_sync_on_server());
                } else {
                    fs::create_directories(abs_path);
                    root->addDirectory(abs_path, fs::last_write_time(abs_path));
                    auto a (root->getDir(abs_path));
                    a->setlast_mod(pt.get<time_t>("last_mod"));
                    pt_write.put("outcome", "1");
                    pt_write.put("path", rel_path);
                    pt_write.put("last_sync", a->getlast_sync_on_server());
                }
            }
            else { //file
                if(fs::exists(abs_path)){
                    if((root->getFile(abs_path))->getCheck()==pt.get<std::string>("checksum")){
                        auto a=(root->getFile(abs_path));
                        a->setlast_mod(pt.get<time_t>("last_mod"));
                        pt_write.put("outcome", "1");
                        pt_write.put("path", rel_path);
                        pt_write.put("last_sync", a->getlast_sync_on_server());
                    } else {
                        pt_write.put("outcome", "0");
                        pt_write.put("path", rel_path);
                    }
                } else {
                    pt_write.put("outcome", "0");
                    pt_write.put("path", rel_path);
                }
            }
            std::ostringstream to_write;
            boost::property_tree::write_json(to_write, pt_write, false);
            //std::cout<<to_write.str()<<std::endl;
            async_write(sock,to_write.str());
        } else if(action=="E"){  //il client ha finito la verifica, il server deve eliminare tutte le cartelle in più,
            // ovvero che non hanno settato last_mod_on_client
            root->endVerify();
        } else if(action=="M"){
            if(type==0){
                auto a=(root->getDir(abs_path));
                a->setlast_mod(pt.get<time_t>("last_mod"));
                pt_write.put("outcome", "1");
                pt_write.put("path", rel_path);
                pt_write.put("last_sync", a->getlast_sync_on_server());
            }else {
                read_file(sock, pt, abs_path, commands);
                auto a=root->getFile(abs_path);
                if(a== nullptr) {
                    root->addFile(abs_path, fs::file_size(abs_path), fs::last_write_time(abs_path));
                    a->setlast_mod(pt.get<time_t>("last_mod"));
                } else {
                    a->setSize(fs::file_size(abs_path));
                    a->setlast_sync(fs::last_write_time(abs_path));
                    a->setlast_mod(pt.get<time_t>("last_mod"));
                    a->setCheck();
                }
                pt_write.put("outcome", "1");
                pt_write.put("path", rel_path);
                pt_write.put("last_sync", a->getlast_sync_on_server());
            }
            std::stringstream to_write;
            boost::property_tree::write_json(to_write, pt_write, false);
            async_write(sock, to_write.str());
        }else if(action=="A"){
            if(type==0){
                fs::create_directories(abs_path);
                root->addDirectory(abs_path, fs::last_write_time(abs_path));
                auto a=root->getDir(abs_path);
                a->setlast_mod(pt.get<time_t>("last_mod"));
                pt_write.put("outcome", "1");
                pt_write.put("path", rel_path);
                pt_write.put("last_sync", a->getlast_sync_on_server());
            } else {
                read_file(sock, pt, abs_path, commands);
                root->addFile(abs_path, pt.get<size_t>("size"), fs::last_write_time(abs_path));
                auto a=root->getFile(abs_path);
                a->setlast_mod(pt.get<time_t>("last_mod"));
                pt_write.put("outcome", "1");
                pt_write.put("path", rel_path);
                pt_write.put("last_sync", a->getlast_sync_on_server());
            }
            std::stringstream to_write;
            boost::property_tree::write_json(to_write, pt_write, false);
            async_write(sock, to_write.str());
        }else if(action=="R"){
            root->remove(abs_path);
            if(type==0){
                fs::remove_all(abs_path);
            } else {
                fs::remove(abs_path);
            }
        }
    }
}

void con_handler::send_backup(std::string& server_path){
    int n=server_path.size();
    for(auto&i: fs::recursive_directory_iterator(server_path)){
        boost::property_tree::ptree pt;
        std::stringstream buf;
        auto pth=i.path().string();
        pth.erase(0, n); //i se si vuole /prova... o i+1 se si vuole prova...
        if(fs::is_directory(i.path())){
            pt.put("path", pth);
            pt.put("type", "0");
            boost::property_tree::write_json(buf, pt);
            write_f(sock, buf.str());
        } else {
            pt.put("path", pth);
            pt.put("type", "1");
            pt.put("size", fs::file_size(i.path()));
            boost::property_tree::write_json(buf, pt);
            write_f(sock, buf.str());
            write_file(sock, i.path().string());
        }
    }
}

void con_handler::handle_write(const boost::system::error_code &err, size_t bytes_transferred) {
    return;
}

void con_handler::finish_syncing(){
    boost::property_tree::ptree pt;
    std::stringstream buf;
    std::shared_ptr<Dir> root;
    boost::filesystem::path s_path(PATH);
    s_path/=owner.get_email();
    std::string server_path=s_path.string();
    std::vector<char> str;
    while(!commands.ended() || !commands.empty())
    {
        auto command = commands.get();
        if(command.has_value()) {
            str = command.value();
        }
        else {
            break;
        }

        buf.str(str.data());
        boost::property_tree::read_json(buf, pt);
        auto action=pt.get<std::string>("action");
        auto type=pt.get<int>("type");
        auto rel_path=pt.get<std::string>("path");
        auto abs_path=server_path+rel_path;
        if(action=="S"){
            if(!fs::exists(abs_path)){
                fs::create_directories(abs_path);
            }
            root=Dir::createRoot(abs_path);
        } else if(action=="V"){
            if(type==0){ //directory
                if(fs::exists(abs_path)){
                    auto last_mod=pt.get<time_t>("last_mod");
                    auto a (root->getDir(abs_path));
                    a->setlast_mod(last_mod);
                } else {
                    fs::create_directories(abs_path);
                    root->addDirectory(abs_path, fs::last_write_time(abs_path));
                    auto a (root->getDir(abs_path));
                    a->setlast_mod(pt.get<time_t>("last_mod"));
                }
            }
            else { //file
                if (fs::exists(abs_path)) {
                    if ((root->getFile(abs_path))->getCheck() == pt.get<std::string>("checksum")) {
                        auto a = (root->getFile(abs_path));
                        a->setlast_mod(pt.get<time_t>("last_mod"));
                    }
                }
            }
        } else if(action=="E"){  //il client ha finito la verifica, il server deve eliminare tutte le cartelle in più,
            // ovvero che non hanno settato last_mod_on_client
            root->endVerify();
        } else if(action=="M"){
            if(type==0){
                auto a=(root->getDir(abs_path));
                a->setlast_mod(pt.get<time_t>("last_mod"));
            }else {
                read_file(sock, pt, abs_path, commands);
                auto a=root->getFile(abs_path);
                if(a== nullptr) {
                    root->addFile(abs_path, fs::file_size(abs_path), fs::last_write_time(abs_path));
                    a->setlast_mod(pt.get<time_t>("last_mod"));
                } else {
                    a->setSize(fs::file_size(abs_path));
                    a->setlast_sync(fs::last_write_time(abs_path));
                    a->setlast_mod(pt.get<time_t>("last_mod"));
                    a->setCheck();
                }
            }

        }else if(action=="A"){
            if(type==0){
                fs::create_directories(abs_path);
                root->addDirectory(abs_path, fs::last_write_time(abs_path));
                auto a=root->getDir(abs_path);
                a->setlast_mod(pt.get<time_t>("last_mod"));
            } else {
                read_file(sock, pt, abs_path, commands);
                root->addFile(abs_path, pt.get<size_t>("size"), fs::last_write_time(abs_path));
                auto a=root->getFile(abs_path);
                a->setlast_mod(pt.get<time_t>("last_mod"));
            }
        }else if(action=="R"){
            root->remove(abs_path);
            if(type==0){
                fs::remove_all(abs_path);
            } else {
                fs::remove(abs_path);
            }
        }
    }
}
