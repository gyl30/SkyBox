cmake_minimum_required(VERSION 3.12)
project(crypt)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fno-strict-aliasing -fno-omit-frame-pointer -g -O0 -ggdb")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fno-strict-aliasing -fno-omit-frame-pointer -g -O0 -ggdb")

if(WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# pthread
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
set(LINK_LIBS Threads::Threads) #

# boost
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_DEBUG_LIBS ON)
set(Boost_USE_RELEASE_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.83.0 REQUIRED COMPONENTS system filesystem context url thread)
include_directories(${Boost_INCLUDE_DIRS})
list(APPEND LINK_LIBS ${Boost_LIBRARIES})
message(STATUS "Found Boost ${Boost_VERSION} include directory ${Boost_INCLUDE_DIR}")

# openssl
set(OPENSSL_USE_STATIC_LIBS ON)
find_package(OpenSSL REQUIRED)
include_directories(SYSTEM ${OPENSSL_INCLUDE_DIR})
list(APPEND LINK_LIBS OpenSSL::SSL OpenSSL::Crypto)
message(STATUS "Found OpenSSL ${OPENSSL_VERSION} include directory ${OPENSSL_INCLUDE_DIR}")

if(WIN32)
    find_package(unofficial-sodium CONFIG REQUIRED)
    find_package(libuv CONFIG REQUIRED)
    include_directories(${Boost_INCLUDE_DIRS})
    list(APPEND LINK_LIBS ${Boost_LIBRARIES} unofficial-sodium::sodium $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>)
    list(APPEND LINK_LIBS ws2_32)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(libsodium REQUIRED IMPORTED_TARGET libsodium)
    list(APPEND LINK_LIBS PkgConfig::libsodium)
    pkg_check_modules(libuv REQUIRED IMPORTED_TARGET libuv)
    list(APPEND LINK_LIBS PkgConfig::libuv)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# rapidjson
include_directories(third/rapidjson/include)

# spdlog
include_directories(third/spdlog/include)

add_subdirectory(client)
add_subdirectory(gui)
add_subdirectory(server)
