cmake_minimum_required(VERSION 3.15)
project(client_project)

set(CMAKE_CXX_STANDARD 20)
find_package(Boost 1.71.0 COMPONENTS system filesystem REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lboost_filesystem -lboost_system")

include_directories(${Boost_INCLUDE_DIRS})
add_executable(client_project main.cpp FileSystem/Dir.cpp FileSystem/Dir.h FileSystem/File.cpp FileSystem/File.h FileSystem/Base.h FileSystem/FileWatcher.h Socket.h hashing/MD5.cpp hashing/MD5.h hashing/SHA256.cpp hashing/SHA256.h Message.cpp Message.h Jobs.h con_handler.cpp con_handler.h Out.h)
if(MINGW)
    target_link_libraries(client_project ws2_32)
endif()
target_link_libraries(client_project ${Boost_LIBRARIES})
