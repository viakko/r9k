set(MODULE_NAME len)
add_executable(${MODULE_NAME} len.c)
target_link_libraries(${MODULE_NAME} PRIVATE tools)