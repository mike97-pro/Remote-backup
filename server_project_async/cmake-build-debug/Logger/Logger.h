//
// Created by mike_ on 14/09/2020.
//

#ifndef PDS_PROJECT_2020_SERVER_LOGGER_H
#define PDS_PROJECT_2020_SERVER_LOGGER_H

#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>


#define LOG_PATH "../Logger/log.txt"

class Logger {
    std::ofstream logFile;
    inline static std::mutex m;

    Logger();
public:
    static std::shared_ptr<Logger> getLogger();

    void write_log(const std::string& buff);

    std::string get_timestamp();

    ~Logger();

    void clean();
};

static std::shared_ptr<Logger> logger;

#endif //PDS_PROJECT_2020_SERVER_LOGGER_H
