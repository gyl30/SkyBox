aux_source_directory(${PROJECT_SOURCE_DIR}/net CMD_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/log CMD_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/protocol CMD_SOURCE_FILE)
add_executable(cmd cmd.cpp application.cpp ${CMD_SOURCE_FILE})

list(APPEND 
    CMD_INCLUDE_DIRS 
    ${PROJECT_SOURCE_DIR}/cmd 
    ${PROJECT_SOURCE_DIR}/net 
    ${PROJECT_SOURCE_DIR}/log
    ${PROJECT_SOURCE_DIR}/protocol)
target_include_directories(cmd PUBLIC "$<BUILD_INTERFACE:${CMD_INCLUDE_DIRS}>")
target_link_libraries(cmd ${LINK_LIBS})
