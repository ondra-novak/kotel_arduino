
set(emul_files emul.cpp)


add_executable(emul ${emul_files})
target_link_libraries(emul kotel ${STANDARD_LIBRARIES} )
