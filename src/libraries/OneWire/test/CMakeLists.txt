
set(one_wire_test_files 
    OneWireTest.cpp
    ../OneWire.cpp    
)


add_executable(one_wire_test ${one_wire_test_files})
target_link_libraries(one_wire_test  ${STANDARD_LIBRARIES} )
