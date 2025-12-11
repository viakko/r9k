set(MODULE_NAME strlen)
add_executable(${MODULE_NAME} strl.c charc.c)
target_link_libraries(${MODULE_NAME} PRIVATE tools)