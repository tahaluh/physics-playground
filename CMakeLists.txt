cmake_minimum_required(VERSION 3.16)
project(PhysicsExperiment)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(X11 REQUIRED)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE APP_SOURCES src/*.cpp)
add_executable(app ${APP_SOURCES})

target_include_directories(app PRIVATE src)

target_include_directories(app PRIVATE ${X11_INCLUDE_DIR})
target_link_libraries(app PRIVATE ${X11_LIBRARIES})