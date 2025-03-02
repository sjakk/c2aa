cmake_minimum_required(VERSION 3.10)

project(HelloWorld)

find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CURL REQUIRED)
add_executable(HelloWorld c2aa.cpp)

target_link_libraries(HelloWorld PRIVATE fmt::fmt)
target_link_libraries(HelloWorld PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(HelloWorld PRIVATE CURL::libcurl)