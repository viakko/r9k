set(MODULE_NAME wdc)
add_executable(${MODULE_NAME} wdc.c)
target_link_libraries(${MODULE_NAME} PRIVATE tools)