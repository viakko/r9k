set(MODULE_NAME url)
add_executable(${MODULE_NAME} url.c)
target_link_libraries(${MODULE_NAME} PRIVATE tools)