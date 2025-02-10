aux_source_directory(${PROJECT_SOURCE_DIR}/net CMD_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/log CMD_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/protocol CMD_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/file CMD_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/crypt CMD_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/util CMD_SOURCE_FILE)

add_executable(server server.cpp application.cpp ${CMD_SOURCE_FILE})

target_link_libraries(server ${LINK_LIBS})
