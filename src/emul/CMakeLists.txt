
set(emul_files emul.cpp
    api/Print.cpp
    api/Stream.cpp
    api/String.cpp
    api/dtostrf.c    
    api/itoa.c
    api/IPAddress.cpp
    wifi/WiFiClient.cpp
    wifi/WiFiServer.cpp
)


add_executable(emul ${emul_files})
target_link_libraries(emul kotel ${STANDARD_LIBRARIES} )
