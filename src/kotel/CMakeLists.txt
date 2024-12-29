
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_LIST_DIR}/web_page.h
    COMMAND ${CMAKE_COMMAND} -DOUT=${CMAKE_CURRENT_LIST_DIR}/web_page.h -P ${CMAKE_CURRENT_LIST_DIR}/to_header.cmake
    DEPENDS ${CMAKE_BINARY_DIR}/src/www/index.html.gz.base64 web_page.h.in to_header.cmake 
    )

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_LIST_DIR}/version.h
    COMMAND ${CMAKE_COMMAND} -DIN=${CMAKE_CURRENT_LIST_DIR}/version.h.in -DOUT=${CMAKE_CURRENT_LIST_DIR}/version.h -P ${CMAKE_CURRENT_LIST_DIR}/gen_version.cmake
    DEPENDS ${CMAKE_SOURCE_DIR}/.git/index
)

SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_LIST_DIR}/web_page.h PROPERTIES GENERATED TRUE)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_LIST_DIR}/version.h PROPERTIES GENERATED TRUE)

add_custom_target(kotel_version DEPENDS ${CMAKE_CURRENT_LIST_DIR}/version.h) 

add_custom_target(kotel_webpage DEPENDS ${CMAKE_CURRENT_LIST_DIR}/web_page.h)
add_dependencies(kotel_webpage kotel_webfiles_compress)

include_directories(BEFORE ${CMAKE_BINARY_DIR})

add_definitions(-DEMULATOR)

set(COMMON_FILES
    kotel.cpp
    http_server.cpp
    controller.cpp
    display_control.cpp
    websocket.cpp
    serial.cpp
    timestamp.cpp
    network_control.cpp
)

add_library(kotel ${COMMON_FILES} )
add_dependencies(kotel kotel_webpage kotel_version)



