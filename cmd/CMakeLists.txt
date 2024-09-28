aux_source_directory(${PROJECT_SOURCE_DIR}/net CLI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/log CLI_SOURCE_FILE)
add_executable(cmd cmd.cpp application.cpp ${CLI_SOURCE_FILE})
list(APPEND 
    CLI_INCLUDE_DIRS 
    ${PROJECT_SOURCE_DIR}/cmd 
    ${PROJECT_SOURCE_DIR}/net 
    ${PROJECT_SOURCE_DIR}/log)
target_include_directories(cmd PUBLIC "$<BUILD_INTERFACE:${CLI_INCLUDE_DIRS}>")
target_link_libraries(cmd ${LINK_LIBS})
