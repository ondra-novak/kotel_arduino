
function convert_to_form_urlencode(req) {
    return Object.keys(req).map(x => {
        return encodeURIComponent(x) + "=" + encodeURIComponent(req[x]);
    }).join('&');
}


function delay(millis) {
    return new Promise(ok => {
        setTimeout(ok, millis);
    });
}

function parse_response(text, sep = "\r\n") {
    return text.split(sep).reduce((obj, line) => {
        let kv = line.split('=', 2).map(x => x.trim());
        if (kv[0]) {
            obj[kv[0]] = kv[1];
        }
        return obj;
    }, {});
}

function decodeBinaryFrame(pattern, buffer) {
    const view = new DataView(buffer);
    let out = {};
    let offset = 0;
    pattern.forEach(x => {
        switch (x[0]) {
            case "uint64": out[x[1]] = view.getBigUint64(offset, true); offset += 8; break;
            case "uint32": out[x[1]] = view.getUint32(offset, true); offset += 4; break;
            case "int16": out[x[1]] = view.getInt16(offset, true); offset += 2; break;
            case "uint16": out[x[1]] = view.getUint16(offset, true); offset += 2; break;
            case "uint8": out[x[1]] = view.getUint8(offset, true); offset += 1; break;
            case "int8": out[x[1]] = view.getInt8(offset, true); offset += 1; break;
            default: throw new TypeError("unknown field type:" + x[0]);
        }
    });
    return out;
}


function encodeBinaryFrame(pattern, data) {

    let arr = null;
    let view = null;
    for (let i = 0; i < 2; ++i) {
        let offset = 0;
        pattern.forEach(x => {
            switch (x[0]) {
                case "uint32": if (view) view.setUInt32(offset, data[x[1]],true); offset += 4; break;
                case "int16": if (view) view.setInt16(offset, data[x[1]],true); offset += 2; break;
                case "uint16": if (view) view.setUint16(offset, data[x[1]],true); offset += 2; break;
                case "uint8": if (view) view.setUint8(offset, data[x[1]],true); offset += 1; break;
                case "int8": if (view) view.setInt8(offset, data[x[1]],true); offset += 1; break;
                default: throw new TypeError("unknown field type:" + x[0]);
            }
        });

        if (arr) break;
        arr = new ArrayBuffer(offset);
        view = new DataView(arr);
    }
    return arr;
}
