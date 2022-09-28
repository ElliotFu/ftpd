cmake_minimum_required(VERSION 3.12)

project(my_ftp_server VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(ftp_src
    main.cpp
    ftp_server.cpp
    command_handler.cpp
    data_handler.cpp
    user_inf.cpp
)

add_executable(my_ftp_server ${ftp_src})
target_link_libraries(my_ftp_server PRIVATE ACE pthread ssl crypto)

file(COPY users.txt DESTINATION ${CMAKE_BINARY_DIR})