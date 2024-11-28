

add_custom_command(OUTPUT ${CMAKE_CURRENT_LIST_DIR}/index.html
                   COMMAND spamake devel ${CMAKE_CURRENT_LIST_DIR}/main.js ${CMAKE_CURRENT_LIST_DIR}/index.html
                   DEPENS index.body.html index.head.html main.js code.js)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/src/www/index.html
                   COMMAND spamake packed ${CMAKE_CURRENT_LIST_DIR}/main.js index.html
                   DEPENS index.body.html index.head.html main.js code.js style.css)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/src/www/index.html.gz.base64
                   COMMAND gzip -cn --best index.html | base64 -w 0  > index.html.gz.base64
                   DEPENDS ${CMAKE_BINARY_DIR}/src/www/index.html)


add_custom_target(kotel_webfiles_compress DEPENDS ${CMAKE_BINARY_DIR}/src/www/index.html.gz.base64 ${CMAKE_CURRENT_LIST_DIR}/index.html)
add_dependencies(kotel_webfiles_compress spamake)




