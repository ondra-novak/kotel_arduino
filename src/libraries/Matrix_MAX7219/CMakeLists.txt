
set(matrix_test_files Matrix_MAX7219.cpp)


add_executable(matrix_test ${matrix_test_files})
target_link_libraries(matrix_test  ${STANDARD_LIBRARIES} )
