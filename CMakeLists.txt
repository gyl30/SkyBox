cmake_minimum_required(VERSION 3.20)

# Windows 平台使用 vcpkg
if(WIN32)
  if(DEFINED ENV{VCPKG_INSTALLATION_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_INSTALLATION_ROOT}/scripts/buildsystems/vcpkg.cmake")
  elseif(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
  endif()
endif()

project(SkyBox VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


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
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GL")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
endif()

# ==================== 依赖 ====================
# Threads
if(NOT WIN32)
  set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  find_package(Threads REQUIRED)
  set(LINK_LIBS Threads::Threads)
endif()

# Boost
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_DEBUG_LIBS ON)
set(Boost_USE_RELEASE_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.83.0 REQUIRED COMPONENTS system filesystem context url thread)
include_directories(${Boost_INCLUDE_DIRS})
list(APPEND LINK_LIBS ${Boost_LIBRARIES})

# Sodium + libuv
if(WIN32)
  find_package(unofficial-sodium CONFIG REQUIRED)
  find_package(libuv CONFIG REQUIRED)
  list(APPEND LINK_LIBS unofficial-sodium::sodium $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv> ws2_32)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(libsodium REQUIRED IMPORTED_TARGET libsodium)
  pkg_check_modules(libuv REQUIRED IMPORTED_TARGET libuv)
  list(APPEND LINK_LIBS PkgConfig::libsodium PkgConfig::libuv)
endif()

# OpenSSL
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
list(APPEND LINK_LIBS OpenSSL::SSL OpenSSL::Crypto)

include_directories(.)
include_directories(third/rapidjson/include)
include_directories(third/spdlog/include)

# ==================== 子项目 ====================
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(gui)

# ==================== 通用设置 ====================
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Install path" FORCE)
endif()

# ==================== 安装规则 ====================
install(TARGETS client DESTINATION client COMPONENT client)
install(TARGETS server DESTINATION server COMPONENT server)
install(TARGETS gclient DESTINATION gui COMPONENT gui)

# ==================== Windows: 使用 windeployqt ====================
if(WIN32)
  find_program(WINDEPLOYQT_EXECUTABLE windeployqt REQUIRED)

  install(CODE [[
    file(TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}/gui/gclient.exe" GCLIENT_EXE)
    if(EXISTS "${GCLIENT_EXE}")
      message(STATUS "Running ${WINDEPLOYQT_EXECUTABLE} on ${GCLIENT_EXE}")
      message(STATUS "${WINDEPLOYQT_EXECUTABLE}  ${GCLIENT_EXE} --dir ${CMAKE_INSTALL_PREFIX}/gui")
      execute_process(COMMAND "${WINDEPLOYQT_EXECUTABLE}" "${GCLIENT_EXE}" --dir "${CMAKE_INSTALL_PREFIX}/gui")
    else()
      message(WARNING "gclient.exe not found at ${GCLIENT_EXE}, skipping windeployqt")
    endif()
  ]])
endif()

# ==================== CPack 打包 ====================
include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "SkyBox")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SkyBox File Transfer & Cloud Storage")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

# Linux: 打 deb 包
if(UNIX AND NOT APPLE)
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "gyl30")
  set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()

# Windows: 打 zip 包
if(WIN32)
  set(CPACK_GENERATOR "ZIP")
  set(CPACK_SET_DESTDIR OFF)
  set(CPACK_PACKAGE_CONTACT "gyl30")
  set(CPACK_ALLOW_ANY_POST_BUILD_DIRECTORY ON)
endif()

include(CPack)
