cmake_minimum_required(VERSION 3.16)
project(GameServerSample CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus /utf-8)
    add_compile_definitions(_WIN32_WINNT=0x0A00 NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)

set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost REQUIRED)

add_executable(game_server
    src/main.cpp
    src/app/ServerApp.cpp
    src/net/Listener.cpp
    src/net/Session.cpp
    src/net/SessionManager.cpp
    src/net/PacketBuffer.cpp
    src/net/PacketBuilder.cpp
    src/net/PacketHandler.cpp
    src/logic/GameWorker.cpp
    src/logic/DbWorker.cpp
    src/user/User.cpp
    src/user/UserManager.cpp
    src/worker/Worker.cpp
    src/worker/WorkerManager.cpp
)

option(GS_ENABLE_METRICS "Enable worker load metrics" OFF)
if(GS_ENABLE_METRICS)
    target_compile_definitions(game_server PRIVATE GS_ENABLE_METRICS GS_LOG_MIN_LEVEL=1)
endif()

target_include_directories(game_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(game_server PRIVATE
    Threads::Threads
    Boost::headers
)