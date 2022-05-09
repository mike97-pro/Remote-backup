//
// Created by franc on 28/04/2020.
//

#include "Dir.h"
#include <iostream>
#include <utility>

Dir::Dir (const std::string& name, std::weak_ptr<Dir> parent, time_t time): parent(std::move(parent)) {
    this->name=name;
    this->last_mod_on_client=time;
    this->last_sync_on_server=0;
    //std::cout<<"Constructor "<<name<<" @ "<<this<<std::endl;
}

Dir::~Dir (){
    //std::cout<<"Destructor "<<name<<" @ "<<this<<std::endl;
}

std::shared_ptr<Dir> Dir::makeDirectory(const std::string& name, std::weak_ptr<Dir> parent, time_t time) {
    std::shared_ptr<Dir> d (new Dir(name, std::move(parent), time)); //creo una directory figlia a cui passo nome e padre
    d->self=d;
    return d; //ritorno directory appena creata
}

std::shared_ptr<Dir> Dir::addDirectory (const std::string& name, time_t time){ //aggiunge la direcotry identificata dal nome alla direcory corrente
    std::lock_guard<std::recursive_mutex> l(this->m);
    if(!name.compare("..") || !name.compare(".") || this->sons.count(name))
       return std::shared_ptr <Dir> ();
    std::shared_ptr<Dir> dir = makeDirectory(name, this->self, time); //chi chiama è il padre
    this->sons.insert(std::pair<std::string, std::shared_ptr<Dir>> (name, dir));
    return dir;
}

std::shared_ptr<File> Dir::addFile (const std::string& name, uintmax_t size, time_t time){
    std::lock_guard<std::recursive_mutex> l(this->m);
    if(!name.compare("..") || !name.compare(".") || this->sons.count(name))
        return std::shared_ptr <File> ();
    std::shared_ptr<File> f (new File(name,size, time));
    this->sons.insert(std::pair<std::string, std::shared_ptr<File>> (name, f));
    return f;
}

std::shared_ptr<Dir> Dir::root = std::shared_ptr<Dir> (nullptr);

std::shared_ptr<Dir> Dir::getRoot(){
    std::lock_guard<std::recursive_mutex> l(root->m);
    return root;
}

std::shared_ptr<Dir> Dir::createRoot(fs::path& p, std::atomic<bool>& running) {
    Dir::root=Dir::makeDirectory(p.string(), std::weak_ptr<Dir>(), fs::last_write_time(p));
    std::lock_guard<std::recursive_mutex> l(root->m);
    Dir::root->parent=Dir::root;
    std::shared_ptr<Dir> current=Dir::root;

    for(auto& i: fs::recursive_directory_iterator(p)) { //fin quando non trova un match torna indietro
        if(!running.load())
            break;
        auto s=normalizza_path(i.path().string());
        while (s.find(current->getName()) == std::string::npos) {
            current = current->getDir("..");
        }
        if (fs::is_directory(i))
            current = current->addDirectory(s, fs::last_write_time(i.path()));
        else
            current->addFile(s, fs::file_size(i), fs::last_write_time(i.path()));
    }

    return Dir::root;
}

std::shared_ptr<Base> Dir::get(const std::string& name){
    std::lock_guard<std::recursive_mutex> l(this->m);
    if(!name.compare(".."))
        return this->parent.lock();
    if(!name.compare("."))
        return this->self.lock();
    if(!name.compare(root->name))
        return root;
    if(this->sons.count(name))
        return this->sons.find(name)->second;
    for(auto& i : this->sons){
        if(i.second->mType()==0) {
            std::shared_ptr<Base> ret = (std::dynamic_pointer_cast<Dir>(i.second))->get(name);
            if (ret != nullptr) {
                return ret;
            }
        }
    }
    return std::shared_ptr<Base> ();
}

std::shared_ptr<Dir> Dir::getDir(const std::string& name){
    std::lock_guard<std::recursive_mutex> l(this->m);
    if(!name.compare(".."))
        return this->parent.lock();
    if(!name.compare("."))
        return this->self.lock();
    if(!name.compare(root->name))
        return root;
    if(this->sons.count(name))
        return (std::dynamic_pointer_cast <Dir> (this->sons.find(name)->second));
    for(auto& i : this->sons){
        if(i.second->mType()==0) {
            std::shared_ptr<Dir> ret = (std::dynamic_pointer_cast<Dir>(i.second))->getDir(name);
            if (ret != nullptr) {
                return ret;
            }
        }
    }
    return std::shared_ptr<Dir> ();
}

std::shared_ptr<File> Dir::getFile(const std::string& name){
    std::lock_guard<std::recursive_mutex> l(this->m);
    if(!name.compare(".."))
        return std::shared_ptr<File>();
    if(!name.compare("."))
        return std::shared_ptr<File>();
    if(this->sons.count(name))
        return std::dynamic_pointer_cast <File> (this->sons.find(name)->second);
    for(auto& i : this->sons){
        if(i.second->mType()==0) {
            std::shared_ptr<File> ret = (std::dynamic_pointer_cast<Dir>(i.second))->getFile(name);
            if (ret != nullptr) {
                return ret;
            }
        }
    }
    return std::shared_ptr<File>();
}

void Dir::ls(int indent) {
    std::lock_guard<std::recursive_mutex> l(this->m);
    for(int i=0;i<indent;i++){
        std::cout<<" ";
    }
    std::cout<<this->name<< " "<<std::ctime(&this->last_mod_on_client)<<std::endl;
    for (auto it=this->sons.begin(); it!=this->sons.end(); it++){
        it->second->ls(indent+4);
    }
}

bool Dir::remove(const std::string& name) {
    std::lock_guard<std::recursive_mutex> l(this->m);
    if(!name.compare(".."))
        return false;
    if(!name.compare("."))
        return false;
    if(!this->sons.count(name)) {
        for(auto& i : this->sons){
            if(i.second->mType()==0) {
                bool ret = (std::dynamic_pointer_cast<Dir>(i.second))->remove(name);
                if(ret)
                    return true;
            }
        }
    } else {
        this->sons.erase(name);
        return true;
    }
    return false;
}

int Dir::mType() {
    std::lock_guard<std::recursive_mutex> l(this->m);
    return 0; //Dir 0 - File 1
}

void Dir::verify_sinch(std::shared_ptr<Jobs<Message>> m, std::atomic<bool>& running) {
    std::lock_guard<std::recursive_mutex> l(this->m);
    Message mdir(this->getName(), "V", std::nullopt, this->mType());
    m->put(mdir);
    for(auto i=this->sons.begin(); i!=this->sons.end() && running; i++) {
        if (i->second->mType() == 0) {
            (std::dynamic_pointer_cast<Dir>(i->second))->verify_sinch(m, running);
        } else {
            auto f=(std::dynamic_pointer_cast<File>(i->second));
            Message mfile(i->second->getName(), "V", f->getCheck(), i->second->mType());
            m->put(mfile);
        }
    }
}

std::string Dir::normalizza_path(std::string p){
    if(p=="/")
        return p;
    fs::path pt(p);
    pt.remove_trailing_separator();
    pt.normalize();
    std::string s=pt.string();
    std::replace(s.begin(), s.end(), '\\', '/');
    return s;
}