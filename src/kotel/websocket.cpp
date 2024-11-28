#include "websocket.h"
#include <array>
#include <random>
#include "sha1.h"
#include "base64.h"



namespace ws {


WsAcceptStr calculate_ws_accept(std::string_view key) {
    SHA1 sha1;
    sha1.update(key);
    sha1.update("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    auto digest = sha1.final();
    WsAcceptStr encoded;
    base64.encode(digest.begin(), digest.end(), encoded.begin());
    return encoded;
}


}




