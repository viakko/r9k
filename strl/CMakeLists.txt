set(MODULE_NAME strl)
add_executable(${MODULE_NAME} strl.c)
target_link_libraries(${MODULE_NAME} PRIVATE tools)