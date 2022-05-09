//
// Created by mike_ on 26/07/2020.
//

#include "UsersDB.h"
#include <vector>
#include <boost/filesystem.hpp>

std::mutex UsersDB::m;

void write_obj(std::fstream &file, std::vector<char> buff){
    size_t size = buff.size();
    file.write((char *)&size,sizeof(size_t));
    file.write(reinterpret_cast<char *>(&buff[0]),size);
}

std::vector<char> read_obj(std::fstream &file){
    size_t size;

    if(!file.read((char *)&size,sizeof(size_t)))
        return std::vector<char>();

    std::vector<char> v(size);
    if(!file.read(reinterpret_cast<char *>(&v[0]),size))
        return std::vector<char>();

    return v;
}

UsersDB::UsersDB() {
    if(!boost::filesystem::exists(DB_PATH))
        boost::filesystem::create_directories(DB_PATH);
    auto p = boost::filesystem::directory_iterator(DB_PATH);
    for(p; p!=boost::filesystem::directory_iterator(); p++){
        if(!boost::filesystem::is_directory(p->path())){
            DB.open(p->path().string(),std::ios::binary | std::ios::in);
            if(!DB.is_open()){
                std::cout<<"Error while opening"<<p->path().filename()<<std::endl;
                exit(-1);
            }
            User obj;
            auto buff = read_obj(DB);
            if(!buff.empty()){
                    obj.deserialize(buff);
                    users.insert({obj.get_email(), obj});
            }
            DB.close();
        }
    }

}

std::shared_ptr<UsersDB> UsersDB::get_DB() {
    std::lock_guard<std::mutex> lock(m);
    if(usersDb)
        return usersDb;
    else {
        usersDb = std::shared_ptr<UsersDB>(new UsersDB());
        return usersDb;
    }
}

std::optional<User> UsersDB::get_user(const std::string& email) {
    std::lock_guard<std::mutex> lock(m);
    auto search = users.find(email);
    if(search!=users.end()){
        return search->second;
    }
    else
        return std::optional<User>();
}

int UsersDB::insert_user(const User &user) {

    if(!get_user(user.get_email()).has_value()) {
        std::lock_guard<std::mutex> lock(m);
        users.insert({user.get_email(),user});


        DB.open(DB_PATH+user.get_email()+".dat",std::ios::binary | std::ios::out);
        if(!DB.is_open()){
            std::cout<<"Error while opening users' file"<<std::endl;
            exit(-1);
        }
        write_obj(DB,user.serialize());

        DB.close();
        return 1;
    }
    else {
        std::cout<<"User "<<user.get_email()<<" already registered."<<std::endl;
        return 0;
    }

}

int UsersDB::remove_user(const std::string& email) {
    auto user = get_user(email);
    std::lock_guard<std::mutex> lock(m);
    if(user.has_value()) {
        users.erase(email);
        remove(std::string(DB_PATH+email+".dat").c_str());
        return 1;
    }
    else
        return -1;

}

void UsersDB::reset_DB() {
    auto p = boost::filesystem::directory_iterator(DB_PATH);
    for(p; p!=boost::filesystem::directory_iterator(); p++){
        boost::filesystem::remove(p->path().c_str());
    }
}

int UsersDB::change_email(const std::string &oldEmail, const std::string &newEmail) {
    auto user = get_user(oldEmail);
    if(user.has_value() && !get_user(newEmail).has_value()){
        remove_user(oldEmail);
        user->set_email(newEmail);
        insert_user(user.value());
        return 1;
    }
    else
        return -1;
}

int UsersDB::change_nick(const std::string &email, const std::string &nick) {
    auto user = get_user(email);
    if(user.has_value()){
        users[email].set_nickname(nick);
        DB.open(DB_PATH+email+".dat",std::ios::binary | std::ios::out);
        write_obj(DB,users[email].serialize());
        DB.close();
        return 1;
    }
    else
        return -1;
}

int UsersDB::change_password(const std::string &email, const std::string &pw) {
    auto user = get_user(email);
    if(user.has_value()){
        users[email].set_password(pw);
        DB.open(DB_PATH+email+".dat",std::ios::binary | std::ios::out);
        write_obj(DB,users[email].serialize());
        DB.close();
        return 1;
    }
    else
        return -1;
}

int UsersDB::set_path(const std::string &email, const std::string &pth) {
    auto user = get_user(email);
    if(user.has_value()){
        users[email].set_path(pth);
        DB.open(DB_PATH+email+".dat",std::ios::binary | std::ios::out);
        write_obj(DB,users[email].serialize());
        DB.close();
        return 1;
    }
    else
        return -1;
}





