cmake_minimum_required(VERSION 3.20)

project(SkyBox VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)

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

if(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
  if(CMAKE_BUILD_TYPE MATCHES Release)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
  endif()

  if(DEFINED ENV{VCPKG_INSTALLATION_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_INSTALLATION_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "Vcpkg toolchain file")
  elseif(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "Vcpkg toolchain file")
  endif()
endif()

# Threads
if(NOT WIN32)
  set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  find_package(Threads REQUIRED)
  set(LINK_LIBS Threads::Threads)
endif()

# Boost
# set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_DEBUG_LIBS ON)
set(Boost_USE_RELEASE_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.86.0 REQUIRED COMPONENTS system filesystem context url thread program_options)
include_directories(${Boost_INCLUDE_DIRS})
list(APPEND LINK_LIBS ${Boost_LIBRARIES})

find_package(PkgConfig REQUIRED)
# Sodium + libuv
if(WIN32)
  find_package(libuv CONFIG REQUIRED)
  pkg_check_modules(libsodium REQUIRED IMPORTED_TARGET libsodium)
  list(APPEND LINK_LIBS PkgConfig::libsodium $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv> ws2_32)
else()
  pkg_check_modules(libsodium REQUIRED IMPORTED_TARGET libsodium)
  pkg_check_modules(libuv REQUIRED IMPORTED_TARGET libuv)
  list(APPEND LINK_LIBS PkgConfig::libsodium PkgConfig::libuv)
endif()

# OpenSSL
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
list(APPEND LINK_LIBS OpenSSL::SSL OpenSSL::Crypto)

include_directories(.)
include_directories(third/rapidjson/include)
include_directories(third/spdlog/include)

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(gui)

install(TARGETS client RUNTIME DESTINATION client)
install(TARGETS gclient RUNTIME DESTINATION gclient)
install(TARGETS server RUNTIME DESTINATION server)
