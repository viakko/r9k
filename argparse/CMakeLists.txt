set(MODULE_NAME argparse)
add_executable(${MODULE_NAME} argparse.c)
target_link_libraries(${MODULE_NAME} PRIVATE tools)