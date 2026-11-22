cmake_minimum_required(VERSION 3.20)
project(libopenundok LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(pugixml REQUIRED)

add_library(openundok
  src/http.cpp
  src/parser.cpp)

target_include_directories(openundok
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/openundok
)

target_link_libraries(openundok
        PRIVATE
        pugixml::pugixml
)

add_executable(demo main.cpp)
target_link_libraries(demo PRIVATE openundok)
