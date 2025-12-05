set(MODULE_NAME clip)
add_executable(${MODULE_NAME} main.c clip.c platform/macos/clipboard_macos.c)
target_link_libraries(${MODULE_NAME} PRIVATE tools "-framework ApplicationServices")