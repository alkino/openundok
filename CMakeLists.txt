cmake_minimum_required(VERSION 3.20)
project(libopenundok LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(pugixml REQUIRED)
find_package(CLI11 REQUIRED)

add_library(openundok
        lib/src/client.cpp
        lib/src/http.cpp
        lib/src/parser.cpp
        lib/src/transport.cpp)

target_include_directories(openundok
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/include/openundok
)

target_link_libraries(openundok
        PRIVATE
        pugixml::pugixml
)

add_executable(opencli cli/main.cpp)
target_link_libraries(opencli PRIVATE openundok CLI11::CLI11)
