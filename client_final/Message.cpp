//
// Created by mentz on 29/07/20.
//

#include "Message.h"
#include "FileSystem/Dir.h"
#include "Socket.h"

Message::Message(const std::string& name, const std::string& action, std::optional<std::string> checksum, int type): name(name), action(action), type(type) {
    boost::property_tree::ptree pt;
    int i = fs::path(Dir::getRoot()->getName()).parent_path().size();
    std::string relative = name;
    relative.erase(0, i); //i se si vuole /prova... o i+1 se si vuole prova...
    if (action == "R" || action=="E" || action=="S") { //se bisogna rimuovere manda R come action e il path
        pt.put("path", relative);
        pt.put("action", action);
        pt.put("type", type);
    } else if (!type) { //se è una directory
        pt.put("path", relative);
        pt.put("action", action);
        pt.put("last_mod", fs::last_write_time(name));
        pt.put("type", type);
    } else if (checksum.has_value()) { //Se è un file e bisogna verificare la sincro manda la checksum
        pt.put("path", relative);
        pt.put("action", action);
        pt.put("checksum", *checksum);
        pt.put("type", type);
        pt.put("last_mod", fs::last_write_time(name));
    } else { //altrimenti bisogna aggiungere il file
        pt.put("path", relative);
        pt.put("action", action);
        pt.put("last_mod", fs::last_write_time(name));
        pt.put("size", fs::file_size(name));
        pt.put("type", type);
    }
    std::stringstream s;
    boost::property_tree::write_json(s, pt, false);
    this->message=s.str();
}

std::string Message::getMessage() {
    return this->message;
}

void Message::consume(boost::shared_ptr<Socket>& soc, std::atomic<bool>& running) {
    if(fs::exists(name) || action=="R" || action=="E") {
        soc->async_write(this->getMessage());
        if ((action == "M" || action == "A") && type) {
            soc->write_file(name, running);
        }
    }
}

Message Message::endVerify() {
    return Message("","E",std::nullopt, -1);
}

Message Message::startVerify(std::string name){
    return Message(name, "S", std::nullopt, -1);
}