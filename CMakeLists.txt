cmake_minimum_required(VERSION 3.15)
project(ServerPostgreSQL++ VERSION 1.0.0)

# ── 1. C++ Standard ───────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # ClangD / clangd-tidy

# ── 2. Dependencies ───────────────────────────────────────────────────────────

find_package(Threads REQUIRED)

# libpqxx: prefer its own CMake package, fall back to pkg-config
find_package(libpqxx QUIET)
if (NOT libpqxx_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBPQXX REQUIRED libpqxx)
endif()

# ── 3. Server executable ──────────────────────────────────────────────────────

set(SOURCES
    src/main.cpp
    src/net/http_types.cpp
    src/net/route.cpp
    src/net/router.cpp
    src/net/server.cpp
    src/thread/thread_pool.cpp
    src/util/dot_env.cpp
    src/db/internal/db_conn.cpp
    src/db/internal/db_pool.cpp
)

add_executable(ServerPostgreSQL++ ${SOURCES})

target_include_directories(ServerPostgreSQL++ PRIVATE
    ${CMAKE_SOURCE_DIR}/include   # existing public headers
    ${CMAKE_SOURCE_DIR}/src       # for db/pg_*.hpp via "db/..." includes
)

if (libpqxx_FOUND)
    target_link_libraries(ServerPostgreSQL++ PRIVATE
        libpqxx::pqxx
        Threads::Threads
    )
else()
    target_include_directories(ServerPostgreSQL++ PRIVATE
        ${LIBPQXX_INCLUDE_DIRS}
    )

    target_link_libraries(ServerPostgreSQL++ PRIVATE
        ${LIBPQXX_LIBRARIES}
        Threads::Threads
    )

    target_compile_options(ServerPostgreSQL++ PRIVATE
        ${LIBPQXX_CFLAGS_OTHER}
    )
endif()
    
target_compile_options(ServerPostgreSQL++ PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# ── 4. Optional: ThreadSanitizer in Debug builds ──────────────────────────────
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(ServerPostgreSQL++ PRIVATE -fsanitize=thread -g)
    target_link_options   (ServerPostgreSQL++ PRIVATE -fsanitize=thread)
endif()