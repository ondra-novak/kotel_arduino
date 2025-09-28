//@html dashboard.html
//@style dashboard.css
//@require temp_gauge.js
//@require tray.js
//@require control_box.js
//@require protocol.js
//@require status.js
//@require config_therm.js
//@require tray_config.js
//@require bind_device.js
//@require header_status.js
//@require power_config.js
//@require menu_settings.js
//@require adv_config.js
//@require net_config.js
//@require chart.js
//@require stats.js


/// <reference path="./temp_gauge.js"/>
/// <reference path="./tray.js"/>
/// <reference path="./control_box.js"/>
/// <reference path="./netstat.js"/>
/// <reference path="./protocol.js"/>
/// <reference path="./status.js"/>
/// <reference path="./config_therm.js"/>
/// <reference path="./tray_config.js"/>
/// <reference path="./bind_device.js"/>
/// <reference path="./header_status.js"/>


class Dashboard {

    #ws = new WebSocketExchange();
    #status = new Status(this.#ws);
    #stats = new Stats(this.#ws);
    #flds = null;
    #fnss = null;
    #menu = new MenuSettings();

    #store_config(cfg) {
        this.#status.set_config(cfg);
    }

    async #update_status_cycle() {
        while (true) {
            const st = await this.#status.wait();
            this.#update_status(st);
        }
    }

    async #update_status(st) {
        const cfg = this.#status.get_last_config();
        const flds = this.#flds;
        if (cfg) {
            flds.temp_out.max = parseInt(cfg.tout);
            flds.temp_out.min = parseInt(cfg.tlsf);
            flds.temp_in.min = parseInt(cfg.tfull);
            flds.temp_in.max = parseInt(cfg.tlow);
            flds.hdr.op = parseInt(cfg.m);
            flds.ssid = cfg["wifi.ssid"];                
            if (!this.#swpd) flds.hdr.op_pend = false;
        }
        if (st) {
            flds.hdr.devicetime =  new Date(parseInt(st.time)*1000);
            flds.rssi = st.rssi;
            const tin = parseInt(st.to) / 10;
            const tout = parseInt(st.ti) / 10;
            flds.temp_out.cur = tin < 0?null: tin;
            flds.temp_in.cur = tout < 0?null: tout;
            flds.temp_out.amp = parseInt(st.tao) / 10;
            flds.temp_in.amp = parseInt(st.tai) / 10;
            flds.tray = parseInt(st.tfp);
            flds.tray.open = !!parseInt(st.tro);
            flds.hdr.simul = !!parseInt(st.tsim);
            flds.hdr.mode = parseInt(st.m);
            flds.hdr.automode = parseInt(st.am);
            flds.tray.open = !!parseInt(st.tro);
            flds.cntr.overheat = !!parseInt(st.mto);
            flds.cntr.pump = !!parseInt(st.p);
            flds.cntr.feeder = !!parseInt(st.fd);
            flds.cntr.fan = !!parseInt(st.fn);
            flds.netfail = false;
            flds.man = st.m == "1";
            flds.pump_active = !parseInt(st.p) ?"":"active";
            flds.fdr_active = !parseInt(st.fd)?"":"active";
            flds.fan_active = !parseInt(st.fn)?"":"active";
            const fnsp =this.#status.get_controls().fns;;
            if (fnsp != this.#fnss) {
                flds.fanpct =  fnsp;
                this.#fnss = fnsp;
            }

        } else {
            flds.netfail = true;
        }
    }

    #swpd = false;

    async fuel(cfgexchange) {
        const r = await configureTray(cfgexchange);
        if (r && r.fuel) {
            let f = r.fuel;
            if (r.unit == "pytel") {
                f = f * this.#status.get_last_config().bgkg;
                const resp = await this.#ws.send_request(WsReqCmd.set_fuel,f.toFixed(0));
                if (resp.length > 0) {
                    alert (resp);
                }
            }
        }
    }

    dashboard_start() {
        this.#ws.ontokenreq =async ()=>{
            const r = await bindDeviceDialog();
            return r.token;
        }
        this.#status.start();
        this.#update_status_cycle();
        const cfgexchange = {
                get:()=>{return this.#status.get_last_config();},
                set:(cfg)=>{return this.#status.set_config(cfg);}
            };

        const f = FormView.load("dashboard");
        f.mount();
        this.#flds = f.get_fields();
        window.test_form = f;
        f.on("temp_out","click",()=>{
            return configureOutputTherm(cfgexchange);
        });
        f.on("temp_in","click",()=>{
            return configureInputTherm(cfgexchange);
        });
        f.on("tray","click",()=>{
            this.fuel(cfgexchange);
        });
        f.on("cntr","click",()=>{
            return powerConfig(cfgexchange);
        });
        f.on("hdr","click",async ()=>{
            const cfg = this.#status.get_last_config();
            const m = 1-cfg.m;
            this.#flds.hdr.op_pend = true;           
            this.#swpd = true;
            await this.#status.set_config({m:m});            
            this.#swpd = false;
            const cntrs = this.#status.get_controls();
            cntrs.fcp = 0;
            cntrs.fnt = 0;
            cntrs.fdt = 0;
        });
        f.on("net","click", ()=>{
            return configureNetwork(cfgexchange);
        });
        f.on("gear","click", async ()=>{
            const r = await this.#menu.open();
            switch (r) {
                case 0: return this.fuel(cfgexchange);
                case 1: return powerConfig(cfgexchange);
                case 2: return configureOutputTherm(cfgexchange);
                case 3: return configureInputTherm(cfgexchange);
                case 4: return configureAdvanced(cfgexchange, this.#ws);
                case 5: return configureNetwork(cfgexchange);
                case 6: return bindConfig(this.#ws);
            }
        });
        f.on("fdr","click", ()=>{
            const cntrs = this.#status.get_controls();
            cntrs.fdt = cntrs.fdt?0:2;
            this.#flds.fdr_active = "pending";
        });
        f.on("fan","click", ()=>{
            const cntrs = this.#status.get_controls();
            cntrs.fnt = cntrs.fnt?0:2;
            this.#flds.fan_active = "pending";
        });
        f.on("pump","click", ()=>{
            const cntrs = this.#status.get_controls();
            cntrs.fcp = cntrs.fcp?0:1;
            this.#flds.pump_active = "pending";
        });
        f.on("fanpct","change",()=>{
            const cntrs = this.#status.get_controls();
            cntrs.fns = this.#flds.fanpct;
        });
        f.on("chart","click",()=>{
            openChart(this.#ws, parseInt(this.#status.get_last_config()["fd.speed"]));
        });
        f.on("stats","click",()=>{
            this.#stats.showDlg();
        });
    }
}