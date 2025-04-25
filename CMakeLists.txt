cmake_minimum_required(VERSION 3.15)
project(SkyBoxCrypt)

# 设置 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 构建类型和调试信息
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
  # 编译器警告和调试选项
  add_compile_options(-Wall -fno-strict-aliasing -fno-omit-frame-pointer -g -O0 -ggdb)
endif()

if(WIN32)
  set(BOOST_TOOLSET "msvc")
  set(CMAKE_POLICY_WARNING_CMP0069 OFF)
  cmake_policy(SET CMP0167 NEW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GL")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
endif()

# 设置 vcpkg 工具链
if(DEFINED ENV{VCPKG_INSTALLATION_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_INSTALLATION_ROOT}/scripts/buildsystems/vcpkg.cmake")
    message(STATUS "Using VCPKG Toolchain from: ${CMAKE_TOOLCHAIN_FILE}")
elseif(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    message(STATUS "Using VCPKG Toolchain from: ${CMAKE_TOOLCHAIN_FILE}")
else()
    message(WARNING "VCPKG_ROOT or VCPKG_INSTALLATION_ROOT not defined, vcpkg integration may fail")
endif()

# 找线程库
if(NOT WIN32)
  set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  find_package(Threads REQUIRED)
endif()

# ================= Boost =================
set(Boost_USE_DEBUG_LIBS ON)
set(Boost_USE_RELEASE_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.83.0 REQUIRED COMPONENTS system filesystem context url thread)

if(Boost_FOUND)
  message(STATUS "Found Boost ${Boost_VERSION} at ${Boost_INCLUDE_DIR}")
  include_directories(${Boost_INCLUDE_DIRS})
else()
  message(FATAL_ERROR "Boost not found!")
endif()

# ================= Sodium + libuv =================
if(WIN32)
  find_package(unofficial-sodium CONFIG REQUIRED)
  find_package(libuv CONFIG REQUIRED)
  list(APPEND LINK_LIBS 
    ${Boost_LIBRARIES}
    unofficial-sodium::sodium
    $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>
    ws2_32
  )
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(libsodium REQUIRED IMPORTED_TARGET libsodium)
  pkg_check_modules(libuv REQUIRED IMPORTED_TARGET libuv)
  list(APPEND LINK_LIBS PkgConfig::libsodium PkgConfig::libuv)
endif()

# ================= OpenSSL =================
find_package(OpenSSL REQUIRED)

if(OpenSSL_FOUND)
  message(STATUS "Found OpenSSL ${OPENSSL_VERSION} at ${OPENSSL_INCLUDE_DIR}")
  include_directories(SYSTEM ${OPENSSL_INCLUDE_DIR})
  list(APPEND LINK_LIBS OpenSSL::SSL OpenSSL::Crypto)
else()
  message(FATAL_ERROR "OpenSSL not found!")
endif()


# ================= 头文件目录 =================
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  third/rapidjson/include
  third/spdlog/include
)

# ================= 子模块 =================
add_subdirectory(client)
add_subdirectory(gui)
add_subdirectory(server)
