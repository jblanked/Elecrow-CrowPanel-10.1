# Add the LCD C module

add_library(usermod_lcd INTERFACE)

target_sources(usermod_lcd INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/lcd_mp.c
    ${CMAKE_CURRENT_LIST_DIR}/lcd.c
    ${CMAKE_CURRENT_LIST_DIR}/font8.c
    ${CMAKE_CURRENT_LIST_DIR}/font12.c
    ${CMAKE_CURRENT_LIST_DIR}/font16.c
    ${CMAKE_CURRENT_LIST_DIR}/font20.c
    ${CMAKE_CURRENT_LIST_DIR}/font24.c
)

target_include_directories(usermod_lcd INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(usermod INTERFACE usermod_lcd)
