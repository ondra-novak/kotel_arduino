execute_process(
    COMMAND git describe --tags
    OUTPUT_VARIABLE VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message("Version: ${VERSION}")

configure_file(
    ${IN}
    ${OUT}
)

