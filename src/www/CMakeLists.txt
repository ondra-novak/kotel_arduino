

set(ALL_FILES index.head.html
              index.body.html
              code.js
              binary_formats.js
              main.js
              utils.js
              websocketclient.js
              style.css)

set(INTERMEDIATE_FILES ${CMAKE_CURRENT_LIST_DIR}/index.html
                ${CMAKE_BINARY_DIR}/src/www/index.html
                ${CMAKE_BINARY_DIR}/src/www/index.html.gz.base64)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/index.html
                          ${CMAKE_BINARY_DIR}/www/index_dev.html
                   COMMAND spamake packed ${CMAKE_CURRENT_LIST_DIR}/main.js index.html
                   COMMAND spamake develsl ${CMAKE_CURRENT_LIST_DIR}/main.js ${CMAKE_BINARY_DIR}/www/index_dev.html
                   DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/index.html.d)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/src/www/index.html.gz.base64
                   COMMAND gzip -cn --best index.html | base64 -w 0  > index.html.gz.base64
                   DEPENDS ${CMAKE_BINARY_DIR}/src/www/index.html)


add_custom_target(kotel_webfiles_compress  DEPENDS ${CMAKE_BINARY_DIR}/src/www/index.html.gz.base64)
add_dependencies(kotel_webfiles_compress spamake)


                   



