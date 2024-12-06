
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

aux_source_directory(${PROJECT_SOURCE_DIR}/qt CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/file CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/crypt CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/log CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/net CLI_GUI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/protocol CLI_GUI_SOURCE_FILE)
add_executable(cli_gui 
    main.cpp 
    widget.cpp
    table_view.cpp
    table_model.cpp
    table_delegate.cpp
    ${CLI_GUI_SOURCE_FILE})
list(APPEND 
    CLI_GUI_INCLUDE_DIRS 
    ${PROJECT_SOURCE_DIR}/qt 
    ${PROJECT_SOURCE_DIR}/file 
    ${PROJECT_SOURCE_DIR}/crypt 
    ${PROJECT_SOURCE_DIR}/log
    ${PROJECT_SOURCE_DIR}/net
    ${PROJECT_SOURCE_DIR}/protocol)
target_include_directories(cli_gui PUBLIC "$<BUILD_INTERFACE:${CLI_GUI_INCLUDE_DIRS}>")
target_link_libraries(cli_gui ${LINK_LIBS} Qt${QT_VERSION_MAJOR}::Widgets)
