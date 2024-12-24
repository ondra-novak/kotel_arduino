execute_process(
    COMMAND git rev-list root..HEAD --count
    OUTPUT_VARIABLE VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message("Version: ${VERSION}")

configure_file(
    ${IN}
    ${OUT}
)

