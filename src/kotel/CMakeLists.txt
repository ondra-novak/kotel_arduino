
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_LIST_DIR}/web_page.h
    COMMAND ${CMAKE_COMMAND} -DOUT=${CMAKE_CURRENT_LIST_DIR}/web_page.h -P ${CMAKE_CURRENT_LIST_DIR}/to_header.cmake
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/../www/code.js
            ${CMAKE_CURRENT_LIST_DIR}/../www/index.html
            ${CMAKE_CURRENT_LIST_DIR}/../www/style.css
            ${CMAKE_CURRENT_LIST_DIR}/web_page.h.in
)

SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_LIST_DIR}/web_page.h PROPERTIES GENERATED TRUE)

add_custom_target(kotel_webpage DEPENDS ${CMAKE_CURRENT_LIST_DIR}/web_page.h)



set(COMMON_FILES
    kotel.cpp
    http_server.cpp
    controller.cpp
    display_control.cpp
    websocket.cpp
    serial.cpp
)

add_library(kotel ${COMMON_FILES} )
add_dependencies(kotel kotel_webpage kotel_webfiles_compress)



