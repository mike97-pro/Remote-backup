//
// Created by franc on 17/07/2020.
//

#ifndef CLIENT_PROJECT_FILEWATCHER_H
#define CLIENT_PROJECT_FILEWATCHER_H

#include <iostream>
#include <boost/filesystem.hpp>
#include <string>
#include <map>
#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <memory>

namespace fs=boost::filesystem;

enum class FileStatus {created, modified, erased};

class FileWatcher {
public:
    std::string path_to_watch;
    // Time interval at which we check the base folder for changes
    std::chrono::duration<int, std::milli> delay;

    // Keep a record of files from the base directory and their last modification time
    FileWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay) : path_to_watch{path_to_watch}, delay{delay} {
        paths_[path_to_watch]=fs::last_write_time(path_to_watch);
        for(auto &file : fs::recursive_directory_iterator(path_to_watch)) {
            auto string=Dir::normalizza_path(file.path().string());
            paths_[string] = fs::last_write_time(file);
        }
    }

    // ...
    // Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
    void start(const std::function<void (std::string, FileStatus, std::shared_ptr<Dir>, std::shared_ptr<Jobs<Message>>)>&
            action, std::atomic<bool>& running, std::shared_ptr<Dir> root, std::shared_ptr<Jobs<Message>> m) {
        while(running) {
            // Wait for "delay" milliseconds
            std::this_thread::sleep_for(delay);


            //Check if root modified
            if(paths_[path_to_watch] != fs::last_write_time(path_to_watch)) {
                paths_[path_to_watch] = fs::last_write_time(path_to_watch);
                action(path_to_watch, FileStatus::modified, root, m);
            }

            auto it = paths_.rbegin();
            while (it != paths_.rend()) {
                if (!fs::exists(it->first)) {
                    action(it->first, FileStatus::erased, root, m);
                    it = std::make_reverse_iterator(paths_.erase(std::next(it).base()));
                }
                else {
                    it++;
                }
            }

            // Check if a file was created or modified
            for(auto &file : fs::recursive_directory_iterator(path_to_watch)) {
                auto current_file_last_write_time = fs::last_write_time(file);
                auto string=Dir::normalizza_path(file.path().string());
                // File creation
                if(!contains(string)) {
                    paths_[string] = current_file_last_write_time;
                    action(string, FileStatus::created, root, m);
                    // File modification
                } else {
                    if(paths_[string] != current_file_last_write_time) {
                        paths_[string] = current_file_last_write_time;
                        action(string, FileStatus::modified, root, m);
                    }
                }
            }
        }
    }
private:
    std::map<std::string, std::time_t> paths_;

    // Check if "paths_" contains a given key
    // If your compiler supports C++20 use paths_.contains(key) instead of this function
    bool contains(const std::string &key) {
        auto el = paths_.find(key);
        return el != paths_.end();
    }
};

auto action=[] (std::string path_to_watch, FileStatus status, std::shared_ptr<Dir> root, std::shared_ptr<Jobs<Message>> m) -> void {

    switch(status) {
        case FileStatus::created:
            if(fs::is_regular_file(path_to_watch)) {
                std::string s=path_to_watch;
                (root->getDir(s.erase(s.find_last_of("/"),std::string::npos)))->addFile(path_to_watch, fs::file_size(path_to_watch), fs::last_write_time(path_to_watch));
                //std::cout << "File created: " << path_to_watch << '\n';
                Message mes(path_to_watch, "A", std::nullopt, 1);
                m->put(mes);
            } else {
                std::string s=path_to_watch;
                (root->getDir(s.erase(s.find_last_of("/"),std::string::npos)))->addDirectory(path_to_watch, fs::last_write_time(path_to_watch));
                //std::cout << "Directory created: " << path_to_watch << '\n';
                Message mes(path_to_watch, "A", std::nullopt, 0);
                m->put(mes);
            }
            break;
        case FileStatus::modified:
            if(fs::is_regular_file(path_to_watch)) {
                //std::cout << "File modified: " << path_to_watch << '\n';
                auto r=root->getFile(path_to_watch);
                r->setlast_mod(fs::last_write_time(path_to_watch));
                r->setSize(fs::file_size(path_to_watch));
                r->setCheck();
                Message mes(path_to_watch, "M", std::nullopt, 1); //il file è da aggiornare
                m->put(mes);
            } else {
                //std::cout << "Dir modified: " << path_to_watch << '\n';
                root->getDir(path_to_watch)->setlast_mod(fs::last_write_time(path_to_watch));
                Message mes(path_to_watch, "M", std::nullopt, 0);
                m->put(mes);
            }
            break;
        case FileStatus::erased:
            if(root->get(path_to_watch)->mType()) {
                //std::cout << "File erased: " << path_to_watch << '\n';
                Message mes(path_to_watch, "R", std::nullopt, 1);
                m->put(mes);
            } else {
                //std::cout << "Directory erased: " << path_to_watch << "\n";
                Message mes(path_to_watch, "R", std::nullopt, 0);
                m->put(mes);
            }
            root->remove(path_to_watch);
            break;
        default:
            std::cout << "Error! Unknown file status.\n";
    }
};

#endif //CLIENT_PROJECT_FILEWATCHER_H
