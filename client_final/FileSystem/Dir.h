//
// Created by franc on 28/04/2020.
//

#ifndef CLIENT_PROJECT_DIR_H
#define CLIENT_PROJECT_DIR_H

#include <string>
#include <memory>
#include <map>
#include "Base.h"
#include "File.h"


class Dir: public Base {
    std::weak_ptr<Dir> parent;
    std::weak_ptr<Dir> self;
    std::map<std::string, std::shared_ptr<Base>> sons;
    Dir (const std::string& name, std::weak_ptr<Dir> parent, time_t time);
    static std::shared_ptr<Dir> root; //oggetto comune a tutte le classi che viene comuqnue eliminato
public:
    static std::shared_ptr<Dir> makeDirectory(const std::string& name, std::weak_ptr<Dir> parent, time_t time);
    static std::shared_ptr<Dir> createRoot(fs::path& p, std::atomic<bool>& running);
    static std::shared_ptr<Dir> getRoot();
    static std::string normalizza_path(std::string p);
    std::shared_ptr<Dir> addDirectory (const std::string& name, time_t time);
    std::shared_ptr<File> addFile (const std::string& name, uintmax_t size, time_t time);
    ~Dir();
    std::shared_ptr<Base> get(const std::string& name);
    void ls (int indent) override ;
    int mType() override ;
    std::shared_ptr<Dir> getDir(const std::string& name);
    std::shared_ptr<File> getFile(const std::string& name);
    bool remove(const std::string& name);
    void verify_sinch(std::shared_ptr<Jobs<Message>> m, std::atomic<bool>& running);
};

#endif //CLIENT_PROJECT_DIR_H
