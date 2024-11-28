set(HTML_FILE ${CMAKE_BINARY_DIR}/../www/index.html.gz.base64)

set(HEADER_TEMPLATE ${CMAKE_CURRENT_LIST_DIR}/web_page.h.in)

file(READ ${HTML_FILE} HTML_CONTENT)

configure_file(${HEADER_TEMPLATE} ${OUT} @ONLY)

