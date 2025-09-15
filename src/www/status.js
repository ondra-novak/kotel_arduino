//@require protocol.js
//@require websocketclient.js
//@require reactive.js

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
        const st = this.#last;
        this.#cur_ctr = parseInt(st.cntr);
        const m = parseInt(st.m);
        const fn = parseInt(st.fn);
        if (m == 2 && fn > 0) this.#req.fns = fn;
        this.run_cycle();        
    }
    start() {
        this.get_config();
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
            if (this.#req.ctr != this.#cur_ctr) {
                this.#req.ctr = this.#cur_ctr+1; 
            }
        });
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
        const resp = await this.#ws.send_request(WsReqCmd.set_config,buildRequest(cfg));
        if (resp) throw new Error(`Failed to set config: ${resp}`);
        return await this.get_config();
    }
       
 }