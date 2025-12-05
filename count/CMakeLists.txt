set(MODULE_NAME count)
add_executable(${MODULE_NAME} count.c charc.c)
target_link_libraries(${MODULE_NAME} PRIVATE tools)