

add_custom_command(OUTPUT code.js.gz.base64
                   COMMAND gzip -cn ${CMAKE_CURRENT_LIST_DIR}/code.js | base64 -w 0  > code.js.gz.base64
                   DEPENDS code.js)

add_custom_command(OUTPUT index.html.gz.base64
                   COMMAND gzip -cn ${CMAKE_CURRENT_LIST_DIR}/index.html | base64 -w 0  > index.html.gz.base64
                   DEPENDS index.html)

add_custom_command(OUTPUT style.css.gz.base64
                   COMMAND gzip -cn ${CMAKE_CURRENT_LIST_DIR}/style.css| base64 -w 0  > style.css.gz.base64
                   DEPENDS style.css)

add_custom_target(kotel_webfiles_compress DEPENDS code.js.gz.base64 index.html.gz.base64 style.css.gz.base64)




