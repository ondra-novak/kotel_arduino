class WebSocketExchange {

    #timeout=10000;
    #ws = null;
    #tosend = [];
    #promises = {};
    #enc = new TextEncoder();
    #pingtm = -1;
    #token = "";
    #ip = false;
    #opentm = null;
    onconnect = function() { };
    ontokenreq = function() {return "";};

    constructor() {
        this.#token = localStorage["token"];
    }

    connect() {
        this.flush();
    }
        
    set_token(tkn) {
        this.#token = tkn
        localStorage["token"] = tkn;
    }
    
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
            this.#ws.close();
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
        }, this.#timeout);
    }

    #clearopentm() {
        if (this.#opentm !== null) {
            clearTimeout(this.#opentm);
            this.#opentm  = null;
        }
    }
    
    flush() {
        if (!this.#ws) {
            let uri = location.href.replace(/^http/, "ws").replace(/(.*)\/.*$/,"$1/") + "api/ws?token="+this.#token;
            this.#ws = new WebSocket(uri);
            this.#ws.binaryType = "arraybuffer";
            this.#ws.onerror = () => {
                this.#clearopentm ();                
                this.reconnect(new TypeError("connection failed"));};
            this.#ws.onclose = async (ev) => {
                this.#clearopentm ();
                if (ev.code > 4000) {
                    let tkn = await this.ontokenreq();
                    this.set_token(tkn);                    
                }
                this.reconnect(new TypeError("connection reset")); 
            };
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
                this.#ip = false;
                this.flush();
            };
            this.#opentm = setTimeout(()=>{
                this.#ws.close();                
                this.reconnect(new TypeError("open timeout")); 
            },this.#timeout);
            this.#ws.onopen = () => {
                this.#clearopentm();
                this.#ip = false;
                this.onconnect();
                this.flush();
            }
        } else if (this.#ws.readyState == WebSocket.OPEN && !this.#ip) {
            if (this.#tosend.length) {
                let x = this.#tosend.shift();
                this.#ws.send(x);
                this.#ip = true;
            }        
        }
    }




}
