set(MODULE_NAME meas)
add_executable(${MODULE_NAME} meas.c)
target_link_libraries(${MODULE_NAME} PRIVATE tools)