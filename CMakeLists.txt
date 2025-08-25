cmake_minimum_required(VERSION 3.20)

project(SkyBox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)
set(CMAKE_VERBOSE_MAKEFILE ON)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

# 构建类型和调试信息
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
  if(NOT WIN32)
    add_compile_options(-Wall -fno-strict-aliasing -fno-omit-frame-pointer -g -O0 -ggdb)
  endif()
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
  if(WIN32)
    add_compile_options(-Wa,-mbig-obj)
  endif()
endif()

if(MSVC)
  add_compile_options(/bigobj /W4)
endif()

find_package(Threads REQUIRED)
if(NOT WIN32)
  set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
endif()

set(Boost_USE_DEBUG_LIBS ON)
set(Boost_USE_RELEASE_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.85.0 REQUIRED COMPONENTS system filesystem context url thread program_options)

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libuv REQUIRED IMPORTED_TARGET libuv-static)

pkg_search_module(SODIUM_HEADERS REQUIRED libsodium)

find_library(SODIUM_STATIC_LIBRARY_FULL_PATH
    NAMES libsodium.a
    HINTS ${SODIUM_HEADERS_LIBRARY_DIRS}
    REQUIRED
)

if(NOT SODIUM_STATIC_LIBRARY_FULL_PATH)
    message(FATAL_ERROR "could not find the full path to libsodium.a")
endif()

add_library(Sodium::Sodium INTERFACE IMPORTED GLOBAL)
set_target_properties(Sodium::Sodium PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SODIUM_HEADERS_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${SODIUM_STATIC_LIBRARY_FULL_PATH};Threads::Threads"
)

set(WIN_LIB "")
if(WIN32)
    list(APPEND WIN_LIB ws2_32 mswsock)
endif()


include_directories(.)
include_directories(third/rapidjson/include)
include_directories(third/spdlog/include)

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(gui)

install(TARGETS client RUNTIME DESTINATION client)
install(TARGETS gclient RUNTIME DESTINATION gclient)
install(TARGETS server RUNTIME DESTINATION server)
