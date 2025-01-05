
set(emul_files emul.cpp
    api/Print.cpp
    api/Stream.cpp
    api/String.cpp
    api/dtostrf.c
    api/itoa.c
    api/IPAddress.cpp
    wifi/WiFiClient.cpp
    wifi/WiFiServer.cpp
    wifi/WiFiS3.cpp
    wifi/UDPClient.cpp
    temp_sim.cpp
    pipe_reader.cpp
    serial_emul.cpp
    SoftwareATSE.cpp
    Matrix_MAX7219_emul.cpp

)


add_executable(emul ${emul_files})
target_link_libraries(emul kotel ${STANDARD_LIBRARIES} )

add_executable(test_matrix Matrix_MAX7219_emul.cpp test_matrix.cpp)
target_link_libraries(test_matrix ${STANDARD_LIBRARIES} )
