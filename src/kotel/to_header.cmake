set(JS_FILE ${CMAKE_BINARY_DIR}/../www/code.js.gz.base64)
set(HTML_FILE ${CMAKE_BINARY_DIR}/../www/index.html.gz.base64)
set(CSS_FILE ${CMAKE_BINARY_DIR}/../www/style.css.gz.base64)

set(HEADER_TEMPLATE ${CMAKE_CURRENT_LIST_DIR}/web_page.h.in)

file(READ ${JS_FILE} JS_CONTENT)
file(READ ${HTML_FILE} HTML_CONTENT)
file(READ ${CSS_FILE} CSS_CONTENT)

configure_file(${HEADER_TEMPLATE} ${OUT} @ONLY)

