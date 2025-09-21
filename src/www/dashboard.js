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
    #flds = null;
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
        } else {
            flds.netfail = true;
        }
        
            
/*
{
    "time": "1758205094",
    "cntr": "0",
    "to": "285",
    "tao": "314",
    "ti": "220",
    "tai": "227",
    "rssi": "-51",
    "tfp": "0",
    "tsim": "0",
    "tis": "0",
    "tos": "0",
    "m": "2",
    "am": "0",
    "tro": "0",
    "mto": "0",
    "p": "1",
    "fd": "0",
    "fn": "60"
}

Ctout=75
tfull=60
tlow=65
touts=10
tins=10
tlsf=55
m=1
fanpc=100
tpump=55
srlog=0
bgkg=15
traykg=225
dspli=0
hval=17.00
fannc=2.00
full.burnout=20
full.fanpw=60
full.fueling=8
low.burnout=30
low.fanpw=40
low.fueling=5
tsinaddr=01-FD-7D-45-38-FC-89-40
tsoutaddr=02-6D-38-D9-78-EF-00-1E
tsincalib60=0
tsoutcalib60=0
wifi.ssid=aaaa
wifi.password=***
net.ip=0.0.0.0
net.dns=0.0.0.0
net.gateway=0.0.0.0
net.netmask=255.255.255.0
fd.Et=0
fuel.Em=0
fd.tefp=0
fd.open=0
fd.lastf=0
fd.speed=251
fd.initf=0
*/

    }

    #swpd = false;

    async fuel(cfgexchange) {
        const r = await configureTray(cfgexchange);
        console.log(r);
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
            }
        });
    }
}