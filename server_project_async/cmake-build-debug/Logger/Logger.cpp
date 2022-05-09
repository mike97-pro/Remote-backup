//
// Created by mike_ on 14/09/2020.
//

#include "Logger.h"
#include <chrono>
#include <ctime>
#include <sstream>

Logger::Logger(){
    logFile.open(LOG_PATH,std::ios::app);
    if(!logFile.is_open())
        throw std::runtime_error("Error while opening log's file");

    logFile<<"\nNew log session "<<get_timestamp()<<"\n";
}

std::shared_ptr<Logger> Logger::getLogger(){
    std::lock_guard<std::mutex> l(m);
    if(logger)
        return logger;
    else{
        logger = std::shared_ptr<Logger>(new Logger());
        return logger;
    }
}

void Logger::clean(){
    std::lock_guard<std::mutex> l(m);
    logFile.close();
    logFile.open(LOG_PATH,std::ios::trunc);
    if(!logFile.is_open())
        throw std::runtime_error("Error while opening log's file");

    logFile<<"\nNew log session "<<get_timestamp()<<"\n";
}

void Logger::write_log(const std::string& buff){
    std::lock_guard<std::mutex> l(m);
    logFile<<buff<<" on "<<get_timestamp();
}

Logger::~Logger(){
    logFile.close();
}

std::string Logger::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto t_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream s;
    s<<std::ctime(&t_c);
    return s.str();
}
