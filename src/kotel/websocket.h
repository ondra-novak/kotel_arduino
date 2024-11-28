#pragma once


#include <array>
#include <cstdint>
#include <string_view>
#include <vector>


namespace ws{

enum class Type: std::uint8_t {
    unknown,
    ///text frame
    text,
    ///binary frame
    binary,
    ///connection close frame
    connClose,
    ///ping frame
    ping,
    ///pong frame
    pong,
    ///continuation frame (only for build)
    continuation
};

struct Message {
    ///contains arbitrary message payload
    /** Data are often stored in the context of the parser and remains valid
     * until next message is requested and parsed.
     */
    std::string_view payload;
    ///type of the frame
    Type type = Type::text;
    ///contains code associated with message connClose
    /** This field is valid only for connClose type */
    std::uint16_t code = 0;
    ///contains true, if this was the last packet of fragmented message
    /**
     * Fragmented messages are identified by fin = false. The type is always
     * set for correct message type (so there is no continuation type on the
     * frontend). If you receive false for the fin, you need to read next message
     * and combine data to one large message.
     *
     * The parser also allows to combine fragmented messages by its own. If this
     * option is turned on, this flag is always set to true
     */
    bool fin = true;
};

///Some constants defined for websockets
struct Base {
public:
	static constexpr unsigned int opcodeContFrame = 0;
	static constexpr unsigned int opcodeTextFrame = 1;
	static constexpr unsigned int opcodeBinaryFrame = 2;
	static constexpr unsigned int opcodeConnClose = 8;
	static constexpr unsigned int opcodePing = 9;
	static constexpr unsigned int opcodePong = 10;

	static constexpr std::uint16_t closeNormal = 1000;
	static constexpr std::uint16_t closeGoingAway = 1001;
	static constexpr std::uint16_t closeProtocolError = 1002;
	static constexpr std::uint16_t closeUnsupportedData = 1003;
	static constexpr std::uint16_t closeNoStatus = 1005;
	static constexpr std::uint16_t closeAbnormal = 1006;
	static constexpr std::uint16_t closeInvalidPayload = 1007;
	static constexpr std::uint16_t closePolicyViolation = 1008;
	static constexpr std::uint16_t closeMessageTooBig = 1009;
	static constexpr std::uint16_t closeMandatoryExtension = 1010;
	static constexpr std::uint16_t closeInternalServerError = 1011;
	static constexpr std::uint16_t closeTLSHandshake = 1015;

};


template<typename Buffer>
class Parser: public Base {
public:

    ///Construct the parser
    /**
     * @param need_fragmented set true, to enable fragmented messages. This is
     * useful, if the reader requires to stream messages. Default is false,
     * when fragmented message is received, it is completed and returned as whole
     */
    constexpr Parser(Buffer &buffer, bool need_fragmented = false)
        :_cur_message(buffer)
        ,_need_fragmented(need_fragmented) {}


    ///push data to the parser
    /**
     * @param data data pushed to the parser
     * @retval false data processed, but more data are needed
     * @retval true data processed and message is complete. You can use
     * interface to retrieve information about the message. To parse
     * next message, you need to call reset()
     */
    constexpr bool push_data(std::string_view data);

    ///Reset the internal state, discard current message
    constexpr void reset();

    ///Retrieve parsed message
    /**
     * @return parsed message
     *
     * @note Message must be completed. If called while message is not yet complete result is UB.
     */
    constexpr Message get_message() const;

    ///Retrieves true, if the message is complete
    /**
     * @retval false message is not complete yet, need more data
     * @retval true message is complete
     */
    constexpr bool is_complete() const {
        return _state == State::complete;
    }


    ///When message is complete, some data can be unused, for example data of next message
    /**
     * Function returns unused data. If the message is not yet complete, returns
     * empty string
     * @return unused data
     */
    constexpr std::string_view get_unused_data() const {
        return _unused_data;
    }

    ///Reset internal state and use unused data to parse next message
    /**
     * @retval true unused data was used to parse next message
     * @retval false no enough unused data, need more data to parse message,
     * call push_data() to add more data
     *
     * @note the function performs following code
     *
     * @code
     * auto tmp = get_unused_data();
     * reset();
     * return push_data(tmp);
     * @endcode
     */
    constexpr bool reset_parse_next() {
        auto tmp = get_unused_data();
        reset();
        return push_data(tmp);
    }



protected:
    enum class State {
        first_byte,
        second_byte,
        payload_len,
        masking,
        payload,
        complete
    };

    std::size_t _state_len = 0;
    std::size_t _payload_len = 0;
    int _mask_cntr = 0;


    Buffer &_cur_message;
    bool _need_fragmented = false;
    bool _fin = false;
    bool _masked = false;

    State _state = State::first_byte;
    unsigned char _type = 0;
    char _masking[4] = {};
    std::string_view _unused_data;

    Type _final_type = Type::unknown;

    constexpr  bool finalize();


    constexpr  void reset_state();
};

template<typename Buffer>
inline constexpr bool Parser<Buffer>::push_data(std::string_view data) {
    std::size_t sz = data.size();
    std::size_t i = 0;
    bool fin = false;
    while (i < sz && !fin) {
        char c = data[i];
        switch (_state) {
            case State::first_byte:
                _fin = (c & 0x80) != 0;
                _type = c & 0xF;
                _state = State::second_byte;        //first byte follows second byte
                break;
            case State::second_byte:
                _masked = (c & 0x80) != 0;
                c &= 0x7F;
                if (c == 127) {
                    _state = State::payload_len;
                    _state_len = 8;                 //follows 8 bytes of length
                } else if (c == 126) {
                    _state = State::payload_len;
                    _state_len = 2;                 //follows 2 bytes of length
                } else if (_masked){
                    _payload_len = c;
                    _state = State::masking;
                    _state_len = 4;                 //follows 4 bytes of masking
                } else  if (c) {
                    _state_len = c;
                    _state = State::payload;        //follows c bytes of payload
                } else {
                    fin = true;         //empty frame - finalize
                }
                break;
            case State::payload_len:
                //decode payload length
                _payload_len = (_payload_len << 8) + static_cast<unsigned char>(c);
                if (--_state_len == 0) { //read all bytes
                     if (_masked) {
                         _state = State::masking;
                         _state_len = 4;        //follows masking
                     } else if (_payload_len) { //non-empty frame
                         _state_len = _payload_len;
                         _state = State::payload;   //read payload
                     } else {
                         fin = true;            //empty frame, finalize
                     }
                }
                break;
            case State::masking:
                _masking[4-_state_len] = c;     //read masking
                if (--_state_len == 0) {        //all bytes?
                    if (_payload_len) {
                        _state_len = _payload_len;
                        _state = State::payload;    //read payload
                        _mask_cntr = 0;
                    } else {
                        fin = true;             //empty frame finalize
                    }
                }
                break;
            case State::payload:
                _cur_message.push_back(c ^ _masking[_mask_cntr]);   //read payload
                _mask_cntr = (_mask_cntr + 1) & 0x3;
                if (--_state_len == 0) {        //if read all
                    fin = true;                 //finalize
                }
                break;
            case State::complete:           //in this state, nothing is read
                _unused_data = data;        //all data are unused
                return true;                //frame is complete
        };
        ++i;
    }
    if (fin) {
        _unused_data = data.substr(i);
        return finalize();
    }
    return false;
}

template<typename Buffer>
constexpr void Parser<Buffer>::reset_state() {
    _state = State::first_byte;
    std::fill(std::begin(_masking), std::end(_masking), static_cast<char>(0));
    _fin = false;
    _masked = false;
    _payload_len = 0;
    _state_len = 0;
    _unused_data = {};
}

template<typename Buffer>
constexpr void Parser<Buffer>::reset() {
    reset_state();
    _cur_message.clear();
}

template<typename Buffer>
constexpr Message Parser<Buffer>::get_message() const {
    if (_final_type == Type::connClose) {
        std::uint16_t code = 0;
        std::string_view message(_cur_message.data(), _cur_message.size());
        if (message.size() >= 2) {
            code = static_cast<unsigned char>(message[0]) * 256 + static_cast<unsigned char>(message[1]);
        }
        if (_cur_message.size() > 2) {
            message = message.substr(2);
        } else {
            message = {};
        }
        return Message {
            message,
            _final_type,
            code,
            _fin
        };
    } else {
        return Message {
            {_cur_message.data(), _cur_message.size()},
            _final_type,
            _type,
            _fin
        };
    }
}



template<typename Buffer>
constexpr bool Parser<Buffer>::finalize() {
    _state = State::complete;
    switch (_type) {
        case opcodeContFrame: break;
        case opcodeConnClose: _final_type = Type::connClose; break;
        case opcodeBinaryFrame:  _final_type = Type::binary;break;
        case opcodeTextFrame:  _final_type = Type::text;break;
        case opcodePing:  _final_type = Type::ping;break;
        case opcodePong:  _final_type = Type::pong;break;
        default: _final_type = Type::unknown;break;
    }

    if (!_fin) {
        if (_need_fragmented) {
            auto tmp = _unused_data;
            reset_state();
            return push_data(tmp);
        }
    }
    return true;
}


template<typename Fn>
inline constexpr bool build(const Message &message, Fn &&output, std::uint8_t *masking = nullptr) {
    std::string_view payload = message.payload;
    char buff[256]= {};

    if (message.type == Type::connClose) {
        char *x = buff;
        *x++ = static_cast<char>(message.code>>8);
        *x++ = static_cast<char>(message.code & 0xFF);
        payload = payload.substr(0, sizeof(buff) - 2);
        std::copy(message.payload.begin(), message.payload.end(),x);
        payload = std::string_view(buff, x - buff);
    }

    // opcode and FIN bit
    char opcode = 0;
    bool fin = message.fin;
    switch (message.type) {
        default:
        case Type::unknown: return false;
        case Type::text: opcode = Base::opcodeTextFrame;break;
        case Type::binary: opcode = Base::opcodeBinaryFrame;break;
        case Type::ping: opcode = Base::opcodePing;break;
        case Type::pong: opcode = Base::opcodePong;break;
        case Type::connClose: opcode = Base::opcodeConnClose;break;
        case Type::continuation: opcode = Base::opcodeContFrame;break;

    }
    output((fin << 7) | opcode);
    // payload length
    std::uint64_t len = payload.size();

    char mm = masking?0x80:0;
    if (len < 126) {
        output(mm| static_cast<char>(len));
    } else if (len < 65536) {
        output(mm | 126);
        output(static_cast<char>((len >> 8) & 0xFF));
        output(static_cast<char>(len & 0xFF));
    } else {
        output(mm | 127);
        output(static_cast<char>((len >> 56) & 0xFF));
        output(static_cast<char>((len >> 48) & 0xFF));
        output(static_cast<char>((len >> 40) & 0xFF));
        output(static_cast<char>((len >> 32) & 0xFF));
        output(static_cast<char>((len >> 24) & 0xFF));
        output(static_cast<char>((len >> 16) & 0xFF));
        output(static_cast<char>((len >> 8) & 0xFF));
        output(static_cast<char>(len & 0xFF));
    }
    char masking_key[4] = {};

    if (masking) {
        for (int i = 0; i < 4; ++i) {
            masking_key[i] = masking[i];
            output(masking_key[i]);
        }
    } else {
        for (int i = 0; i < 4; ++i) {
            masking_key[i] = 0;
        }
    }

    int idx =0;
    for (char c: payload) {
        c ^= masking_key[idx];
        idx = (idx + 1) & 0x3;
        output(c);
    }
    return true;

}

class WsAcceptStr: public std::array<char, 28> {
public:
    using std::array<char, 28>::array;
    operator std::string_view() const {return {data(),size()};}
};
///calculate WebSocket Accept header value from key
/**
 * @param key content of Key header
 * @return content of Accept header
 */
WsAcceptStr calculate_ws_accept(std::string_view key);



}
