cmake_minimum_required(VERSION 3.15)
project(Server++ VERSION 1.0.0)

# --- 1. Setup C++ Standard ---
# Ensures strict standard compliance
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

# ClangD compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- 2. Define Sources ---
set(SOURCES
    src/main.cpp
    src/net/http_types.cpp
    src/net/route.cpp
    src/net/router.cpp
    src/net/server.cpp
    src/util/dot_env.cpp
)

# --- 3. Create Executable ---
add_executable(Server++ ${SOURCES})

# --- 4. Include Directories ---
target_include_directories(
    Server++ PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)