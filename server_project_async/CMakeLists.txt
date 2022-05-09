cmake_minimum_required(VERSION 3.15)
project(pds_project_2020_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS -pthread)
add_executable(pds_project_2020_server main.cpp Database/User.cpp Database/User.h hashing/MD5.cpp hashing/MD5.h hashing/SHA256.cpp hashing/SHA256.h Database/UsersDB.cpp Database/UsersDB.h Server.cpp Server.h con_handler.cpp con_handler.h Queue.h fs/Dir.h fs/Dir.cpp fs/File.h fs/File.cpp fs/Base.h mutex_print.h Logger/Logger.cpp Logger/Logger.h)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lboost_filesystem -lboost_system")

find_package(Boost COMPONENTS system filesystem REQUIRED)
if(MINGW)
    target_link_libraries(pds_project_2020_server ws2_32)
    target_link_libraries(pds_project_2020_server wsock32)
endif()
if(Boost_FOUND)
    message(STATUS "Boost found")
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(pds_project_2020_server ${Boost_LIBRARIES})
endif()



