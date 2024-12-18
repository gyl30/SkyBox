aux_source_directory(${PROJECT_SOURCE_DIR}/file CLI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/crypt CLI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/log CLI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/net CLI_SOURCE_FILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/protocol CLI_SOURCE_FILE)

add_executable(client client.cpp ${CLI_SOURCE_FILE})

target_link_libraries(client ${LINK_LIBS})
