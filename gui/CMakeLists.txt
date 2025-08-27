set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

aux_source_directory(${PROJECT_SOURCE_DIR}/gui CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/file CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/crypt CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/log CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/net CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/config CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/protocol CLI_GUI_SOURCE_FILE)

set(WIN32_FLAG "")
if(WIN32)
    set(WIN32_FLAG WIN32)
endif()

add_executable(gclient ${WIN32_FLAG} ${CLI_GUI_SOURCE_FILE})

target_link_libraries(gclient 
    Qt6::Widgets
    Threads::Threads 
    OpenSSL::SSL 
    OpenSSL::Crypto 
    Boost::system
    Boost::filesystem
    Boost::context
    Boost::url
    Boost::thread
    Boost::program_options
    PkgConfig::libuv
    Sodium::Sodium
    ${WIN_LIB}
)
