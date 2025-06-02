set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

aux_source_directory(${PROJECT_SOURCE_DIR}/gui CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/file CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/crypt CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/log CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/net CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/config CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/protocol CLI_GUI_SOURCE_FILE)

set(WIN32_FLAG "")
if(MSVC)
    set(WIN32_FLAG WIN32)
endif()

add_executable(gclient ${WIN32_FLAG}
    ${CLI_GUI_SOURCE_FILE})

target_link_libraries(gclient ${LINK_LIBS} Qt${QT_VERSION_MAJOR}::Widgets)
