//@require forms.js
//@require websocketclient.js
//@require protocol.js
//@style stats.css
//@html stats.html

/// <reference path="./protocol.js"/>
/// <reference path="./websocketclient.js"/>
/// <reference path="./forms.js"/>


function formatSeconds(s) {
    const seconds = parseInt(s, 10);
    const min = Math.floor(seconds / 60);
    const hour = Math.floor(min / 60);
    const day = Math.floor(hour / 24);

    const remMin = min % 60;
    const remHour = hour % 24;

    if (day > 0) {
        return `${day}d ${remHour.toString().padStart(2, '0')}:${remMin.toString().padStart(2, '0')}`;
    } else {
        return `${remHour.toString().padStart(2, '0')}:${remMin.toString().padStart(2, '0')}`;
    }
}

class Stats {
    #last_stats = {};
    #id = null;
    #ws;

    constructor(ws) {
        this.#ws = ws;
    }

    async #fetch() {
        const data = await this.#ws.send_request(WsReqCmd.get_stats);
        this.#last_stats = parseResponse(data);
    }

    showDlg() {
        const dlg = ModalDialog.load("statsDlg");
        const recalc = ()=>{
                const lst = this.#last_stats
                const st = Object.keys(lst)
                        .reduce((r,k)=>{
                            const v = this.#last_stats[k];
                            if (k.startsWith("rt.")) {
                                const seconds = parseInt(v);
                                r[k] = formatSeconds(seconds);
                            } else {
                                r[k] = v;
                            }
                            return r;
                        }, {}); 
                const rtt = parseInt(lst["rt.full_power"])
                          +  parseInt(lst["rt.low_power"])
                          +  parseInt(lst["rt.cooling"]);
                const consumed = parseInt(lst["rt.feeder"])/parseInt(lst["tray.feeder_speed"]);
                st.feeder_fuel_kg = consumed.toFixed(0);
                st.avg_fuel_con = (consumed * 86400 / rtt).toFixed(0);
                st.feeder_speed_calc = (parseInt(lst["tray.feeder_time_accum"])
                            / parseInt(lst["tray.fuel_kg_accum"])).toFixed(0)
                dlg.set(st);
        };
        recalc();
        this.#fetch().then(()=>recalc(),()=>recalc());
        const id = setInterval(()=>{
            this.#fetch().then(()=>recalc(),()=>{});
        },4000);
        return dlg.do_modal("bst")
            .then(x=>{
                clearInterval(id); 
                return x;
            });
    }

}

