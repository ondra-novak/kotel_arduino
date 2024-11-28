class WebSocketExchange {

    #ws = null;
    #tosend = [];
    #promises = {};
    #enc = new TextEncoder();
    #pingtm = -1;
    onconnect = function() { };

    send_request(cmd, content) {
        return new Promise((ok, err) => {
            if (typeof cmd == "string") cmd = this.#enc.encode(cmd);
            else if (typeof cmd == "number") cmd = Uint8Array.from([cmd]);
            else return Promise.reject(new TypeError("invalid command format"));
            if (typeof content == "string") content = this.#enc.encode(content);
            else if (!content instanceof ArrayBuffer) {
                if (Array.isArray(content)) content = Uint8Array.from(content);
                else Promise.reject(new TypeError("invalid content format"));
            } else {
                content = new Uint8Array(content);
            }
            let selector = cmd[0];
            var mergedArray = new Uint8Array(cmd.length + content.length);
            mergedArray.set(cmd, 0);
            mergedArray.set(content, cmd.length);
            if (!this.#promises[selector]) {
                this.#promises[selector] = [];
            }
            this.#promises[selector].push([ok, err]);
            this.#tosend.push(mergedArray);
            this.flush();
        });
    }

    reconnect(err) {
        if (this.#ws) {
            Object.keys(this.#promises).forEach(x => this.#promises[x].forEach(z => z[1](err)));
            this.#promises = {};
            this.#ws = null;
            clearTimeout(this.#pingtm);
            setTimeout(() => this.flush(), 1000);
        }
    }

    reset_timeout() {
        if (this.#pingtm >= 0) clearTimeout(this.#pingtm);
        this.#pingtm = setTimeout(() => {
            this.#pingtm = -1;
            this.#ws.close();
            this.reconnect(new TypeError("connection timeout"));
        }, 5000);
    }

    flush() {
        if (!this.#ws) {
            this.#ws = new WebSocket(location.href.replace(/^http/, "ws") + "api/ws");
            this.#ws.binaryType = "arraybuffer";
            this.#ws.onerror = () => { this.reconnect(new TypeError("connection failed")); };
            this.#ws.onclose = () => { this.reconnect(new TypeError("connection reset")); };
            this.#ws.onmessage = (ev) => {
                this.reset_timeout();
                let data = ev.data;
                let selector;
                if (typeof data == "string") {
                    selector = this.#enc.encode(data)[0];
                    data = data.substr(1);
                } else {
                    data = new Uint8Array(data);
                    selector = data[0];
                    data = data.slice(1).buffer;

                }
                let p = this.#promises[selector];
                if (p && p.length) {
                    let q = p.shift();
                    q[0](data);
                }
            };
            this.#ws.onopen = () => {
                this.onconnect();
                this.flush();
            }
        } else if (this.#ws.readyState == WebSocket.OPEN) {
            this.#tosend.forEach(x => this.#ws.send(x));
            this.#tosend = [];
        }
    }




}
