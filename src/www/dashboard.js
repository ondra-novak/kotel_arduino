//@html dashboard.html
//@style dashboard.css
//@require temp_gauge.js
//@require tray.js
//@require control_box.js
//@require netstat.js
//@require protocol.js
//@require status.js
//@require config_therm.js
//@require tray_config.js
//@require bind_device.js
//@require header_status.js


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

    #store_config(cfg) {
        this.#status.set_config(cfg);
    }

    async #update_status() {
        while (true) {
            const st = await this.#status.wait();
            const cfg = this.#status.get_last_config();
            const flds = this.#flds;
            if (cfg) {
                flds.temp_out.max = parseInt(cfg.tout);
                flds.temp_out.min = parseInt(cfg.tlsf);
                flds.temp_in.min = parseInt(cfg.tfull);
                flds.temp_in.max = parseInt(cfg.tlow);
            }
/*

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
    }


    dashboard_start() {
        this.#ws.ontokenreq =async ()=>{
            const r = await bindDeviceDialog();
            return r.token;
        }
        this.#status.start();
        this.#update_status();

        const f = FormView.load("dashboard");
        f.mount();
        this.#flds = f.get_fields();
        window.test_form = f;
        f.on("temp_out","click",()=>{

        });
        f.on("temp_in","click",()=>{

        });
        f.on("tray","click",()=>{

        });
        f.on("cntr","click",()=>{

        });
        f.on("netstat","click",()=>{

        });
    }
}