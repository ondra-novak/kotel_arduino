//@require protocol.js
//@require websocketclient.js
//@require reactive.js

/// <reference path="./websocketclient.js"/>
/// <reference path="./reactive.js"/>
/// <reference path="./protocol.js"/>

class Status {

    #ws;
    #last = {};
    #wt;
    #ok;
    #req = {
        fdt:0,
        fnt:0,
        fns:50,
        fcp:0,
        ctr:0
    }
    #cur_ctr = 0;
    #config;

    
    constructor(ws) {
        this.#ws = ws;
        this.#ws.onconnect = () => {
            this.get_config();
        }
        
    }
    async cycle() {
        try {
            const resp = await this.#ws.send_request(WsReqCmd.monitor_cycle,buildRequest(this.#req));        
            this.#last = parseResponse(resp);;
            this.#ok(this.#last);
        } catch (e) {
            this.#ok(null);
            console.warn(e);        
        }
        this.#wt = null;
        const st = this.#last;
        const cntr = parseInt(st.cntr);
        this.#cur_ctr = cntr;            
        const m = parseInt(st.m);
        const fn = parseInt(st.fn);
        if (m == 2 && fn > 0) this.#req.fns = fn;
        if (m == 1 && cntr > this.#req.ctr) {
            this.#req.fdt = 2*parseInt(st.fd);
            this.#req.fnt = 2*parseInt(st.fn);
            this.#req.fcp = parseInt(st.p);
            if (this.#req.fnt) this.#req.fns = fn;
        }
        this.run_cycle();        
    }
    start() {
        this.run_cycle();
    }
    
    run_cycle() {
        this.#wt = new Promise(x=>this.#ok = x);
        setTimeout(()=>this.cycle(), 1000);        
    }
    
    get() {
        return this.#last;
    }
    get_controls() {
        return reactive(this.#req, ()=>{            
            this.#req.ctr = this.#cur_ctr + 1; 
            
        });
    }

    is_man_inactive() {
        return this.#cur_ctr != this.#req.ctr;
    }
    
    async wait() {
        return await this.#wt;
    }
    async get_config() {
        const resp = await this.#ws.send_request(WsReqCmd.get_config);
        const cfg = parseResponse(resp);
        this.#config = cfg;
        return cfg;   
    }   
    
    get_last_config() {
        return this.#config;
    }
    
    async set_config(cfg) {
        let resp;
        try {
             resp = await this.#ws.send_request(WsReqCmd.set_config,buildRequest(cfg));
        } catch (e) {
            console.warn(e);            
            return this.set_config(cfg);
        }
        if (resp) throw new Error(`Failed to set config: ${resp}`);
        this.#config = Object.assign(this.#config, cfg);
        return this.#config;
    }
       
 }