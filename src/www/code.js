//@require utils.js
//@require websocketclient.js
//@require binary_formats.js
//@require computing.js

let connection = new WebSocketExchange();

let ids = Array.prototype.reduce.call(document.querySelectorAll("[id]"), (a,x)=>{
   a[x.id] = x;
   return a;
},{});

let Controller = {
    man: {
        /*        force_pump: false,
                feeder: false,
                fan: false,
                fan_speed: null,*/
    },

    status: {},
    config: {},
    stats: {},
    _promises: {},

    update_status_cycle: async function() {
        if (this._status_timer) clearTimeout(this._status_timer);
        try {
            let req = {};
            if (this.man.feeder !== undefined) {
                req.feeder_timer = this.man.feeder ? 3 : 0;
                if (!this.man.feeder) delete this.man.feeder;
            } else {
                req.feeder_timer = 255;
            }
            if (this.man.fan !== undefined) {
                req.fan_timer = this.man.fan ? 3 : 0;
                if (!this.man.fan) delete this.man.fan;
            } else {
                req.fan_timer = 255;
            }

            if (this.man.fan_speed !== undefined) {
                req.fan_speed = this.man.fan_speed ? this.man.fan_speed : 0;
                delete this.man.fan_speed;
            } else {
                req.fan_speed = 255;
            }
            if (this.man.force_pump !== undefined) {
                req.force_pump = this.man.force_pump ? 1 : 0;
                delete this.man.force_pump;
            } else {
                req.force_pump = 255;
            }
            let data = await connection.send_request("c", encodeBinaryFrame(ManualControlWs, req));
            let out = decodeBinaryFrame(StatusOutWs, data);
            if (out.temp_output_value < -10000) delete out.temp_output_value;
            else out.temp_output_value = out.temp_output_value * 0.1;
            if (out.temp_input_value < -10000) delete out.temp_input_value;
            else out.temp_input_value = out.temp_input_value * 0.1;
            out.temp_output_amp_value = out.temp_output_amp_value * 0.1;
            out.temp_input_amp_value = out.temp_input_amp_value * 0.1;
            out.time = new Date(Number(out.timestamp)*1000);
            this.status = out;
            this.on_status_update(out);
        } catch (e) {
            this.on_error("status", e);
        }
        this._status_timer = setTimeout(this.update_status_cycle.bind(this), 1000);
    },

    update_stats_cycle: async function() {
        if (this._stat_timer) killTimer(this._stat_timer);
        try {
            let resp = await connection.send_request("T", []);
            this.stats = decodeBinaryFrame(StatsOutWs, resp);
            this.on_stats_update(this.stats);
        } catch (e) {
            this.on_error("stats", e);
        }
        this._stats_timer = setTimeout(this.update_stats_cycle.bind(this), 30000);
    },

    read_config: async function() {
        while (true) {
            try {
                this.config = parse_response(await connection.send_request('C', {}));
                return this.config;
            } catch (e) {
                this.on_error("config", e);
            }
            await delay(1000);
        }
    },



    on_status_update: function(x) { console.log("status", x); },
    on_stats_update: function(x) { console.log("stats", x); },
    on_error: function(x, y) { console.error(x, y); },

    set_config: async function(config) {
        if (Object.keys(config).length == 0) return;
        while (true) {
            try {
                let body = convert_to_form_urlencode(config);
                let resp = await connection.send_request("S", body);
                let config_conv = parse_response(body, "&");
                Object.assign(this.config, config_conv);
                return true;
            } catch (e) {
                console.error(e);
            }
            await delay(1000);

        }
    }



};

function update_config_form(cfg) {
    Object.keys(cfg).forEach(k => {
        const v = cfg[k];
        document.getElementsByName(k).forEach(x => {
            x.value = v;
        })
    });
}


function update_temperature(id, temp, ref_temp, ampl_temp) {
    const el = document.getElementById(id);
    const cur = el.getElementsByClassName("cur")[0];
    const ref = el.getElementsByClassName("ref")[0];
    const label = el.getElementsByClassName("cur_temp")[0];
    const ampl = el.getElementsByClassName("amp_temp")[0];

    temp = parseFloat(temp);
    ref_temp = parseFloat(ref_temp);
    ampl_temp = parseFloat(ampl_temp);


    function angle(temp) {
        let a = (temp - 20) * 270 / 80;
        if (a < 0) a = 0;
        if (a > 270) a = 270;
        return (a - 45) + "deg";
    }

    cur.hidden = isNaN(temp);
    cur.setAttribute("style", "transform: rotate(" + angle(temp) + ");");
    ref.setAttribute("style", "transform: rotate(" + angle(ref_temp) + ");");
    label.textContent = isNaN(temp) ? "--.-" : temp.toFixed(1);
    ampl.textContent = ampl_temp.toFixed(1);
}

function update_fuel(id, value) {
    const el = document.getElementById(id);
    const pb = el.getElementsByClassName("fill")[0];
    const label = el.getElementsByClassName("label")[0];
    const max_tray = Controller.config.traykg;

    if (value > max_tray) value = max_tray;
    const koef = 0.7;
    const p = value / max_tray;
    const f = Math.floor((Math.sqrt(koef * p) + p * (1 - Math.sqrt(koef))) * 100);
    const fstr = f + "%";
    pb.setAttribute("style", "height:" + fstr);
    label.textContent = isFinite(p)?(p*100).toFixed(0):"---";
}

function calculate_fuel_remain() {
    const fill_time = parseFloat(Controller.stats["tray_fill_time"]);
    const cur_time = parseFloat(Controller.stats["feeder_time"]);
    const consup = parseFloat(Controller.config["tray.f1kgt"]);
    const fillup = parseFloat(Controller.config["tray.tfkg"]);

    const ellapsed = cur_time - fill_time;
    const consumed = ellapsed / consup;
    const remain = fillup < consumed ? 0 : fillup - consumed;

    return remain;


}

function close_window() {
    this.parentElement.parentElement.hidden = true;
}

function show_error(win, error) {
    const lst = win.getElementsByClassName("chyba");
    Array.prototype.forEach.call(lst, el => {
        if (!error | el.classList.contains(error)) {
            el.classList.add("zobrazena");
        }
    });
}
function hide_error(win) {
    const lst = win.getElementsByClassName("chyba");
    Array.prototype.forEach.call(lst, el => {
        el.classList.remove("zobrazena");
    });
}


function unhide_section() {
    this.parentElement.classList.toggle("unhidden");
}

async function nastav_teplomer(field) {
    let win = ids["nastav_hw_teplomer"];
    let btn = win.getElementsByTagName("button");
    win.hidden = false;
    btn[0].onclick = () => {
        let cfg = {};
        cfg[field] = selected;
        btn[0].disabled = true;
        Controller.set_config(cfg);
        win.hidden = true;
    };
    btn[0].disabled = true;
    let list = win.getElementsByClassName("list")[0];
    list.innerHTML = "";
    let resp = await fetch("/api/scan_temp", {
        method: "post"
    });
    let selected = "";
    if (resp.ok) {
        resp = parse_response(await resp.text());
        Object.keys(resp).forEach(addr => {
            let temp = resp[addr];
            let sp = document.createElement("span");
            let lb = document.createElement("label");
            let elem = document.createElement("input");
            elem.setAttribute("type", "radio");
            elem.setAttribute("name", "t");
            elem.onchange = () => {
                selected = addr;
                btn[0].disabled = false;
            };
            sp.appendChild(elem);
            sp.appendChild(document.createTextNode(temp));
            lb.appendChild(sp);
            list.appendChild(lb);
            lb.appendChild(document.createTextNode(addr));
        });
    }
}

function dialog_nastaveni_teploty(field, hw_field, trend_field) {
    let el = ids["nastav_teplotu"];
    hide_error(el);
    el.hidden = false;
    el.dataset.field = field;
    el.dataset.hwfield = hw_field;
    let imp = el.getElementsByTagName("input")[1];
    let trnd = el.getElementsByTagName("input")[0];
    imp.value = Controller.config[field];
    trnd.value = (parseFloat(Controller.config[trend_field])/6).toFixed(1);
    let btm = el.getElementsByTagName("button");
    btm[0].onclick = () => {
        nastav_teplomer(hw_field);
    };
    btm[1].onclick = async () => {
        let cfg = {};
        let val = imp.valueAsNumber;
        let trnd_val = trnd.valueAsNumber;
        if (isNaN(val)) {
            show_error(el, "prazdne")
        } else if (val < 30) {
            show_error(el, "male")
        } else if (val > 90) {
            show_error(el, "velke")
        } else if (isNaN(trnd_val)) {
            show_error(el, "trnd_prazdne")
        } else if (trnd_val < 0) {
            show_error(el, "trnd_male")
        } else if (trnd_val > 25) {
            show_error(el, "trnd_velke")
        } else {
            cfg[field] = val;
            cfg[trend_field] = (trnd_val*6).toFixed(0);
            btm[1].disabled = true;
            await Controller.set_config(cfg);
            el.hidden = true;
        }
    };
    btm[1].disabled = false;
}

function nastav_parametry(id) {
    let win = document.getElementById(id);
    hide_error(win);
    win.hidden = false;
    let inputs = win.getElementsByTagName("input");
    let button = document.querySelector("button[name=ok]");
    Array.prototype.forEach.call(inputs, x => {
        x.value = Controller.config[x.name];
    });
    button.onclick = async () => {
        let cfg = {};
        let err = false;
        Array.prototype.forEach.call(inputs, x => {
            if (x.getAttribute("type") == "number") {
                let val = x.valueAsNumber;
                let name = x.name;
                if (isNaN(val)
                    || (val < parseFloat(x.getAttribute("min")))
                    || (val > parseFloat(x.getAttribute("max")))) {
                    err = true;
                    x.classList.add("valerror")
                } else {
                    if (!x.dataset.local) cfg[name] = val;
                    x.classList.remove("valerror")
                }
            }
        });
        if (err) {
            show_error(win, "fail");
        } else {
            button.disabled = true;
            await Controller.set_config(cfg);
            button.disabled = false;
            win.hidden = true;
        }
    }
    return win;
}



function nastav_horeni() {
    let win = nastav_parametry("nastav_topeni");
    power_conv_init(win);
}

async function nastav_wifi() {
    let win = ids["nastav_wifi"];
    hide_error(win);
    win.hidden = false;
    let inputs = win.getElementsByTagName("input");
    let buttons = win.getElementsByTagName("button");
    Array.prototype.forEach.call(inputs, x => {
        x.value = Controller.config[x.name] || "";
    });
    let current_password = Controller.config["wifi.password"];
    inputs[0].checked = Controller.config["net.ip"] == "0.0.0.0";
    buttons[1].onclick = async () => {
        let cfg = {};
        let err = false;
        Array.prototype.forEach.call(inputs, x => {
            if (x.getAttribute("type") == "text") {
                let val = x.value;
                let name = x.name;
                if (name.startsWith("net.")) {
                    const fld = val.split('.');
                    const e = fld.find(x => {
                        const v = parseInt(x);
                        return isNaN(v) || v < 0 || v > 255;
                    });
                    if (e !== undefined) err = true;
                }
                cfg[name] = val;
            }
        });
        if (err) {
            show_error(win, "addrerror");
        } else {
            if (cfg["wifi.password"] == current_password) {
                delete cfg["wifi.password"];
            }
            if (inputs[0].checked) {
                cfg["net.ip"] = "0.0.0.0";
            }
            buttons[0].disabled = true;
            await Controller.set_config(cfg);
            buttons[0].disabled = false;
            win.hidden = true;
        }
    }
    buttons[0].onclick = ()=>{
        win.hidden = true;
        dialog_show_code();
    }
}


function power_to_params(vyhrevnost_el, f1kgt_el, power_el, fueling_el, burnout_el, low_power) {
    const MJ_kg = vyhrevnost_el.valueAsNumber;
    const skg = f1kgt_el.valueAsNumber;
    const pw = power_el.valueAsNumber;
    const cycle_f = calculate_fuel_transport_time(MJ_kg, skg, pw ,1);
    let df;
    if (low_power) {
        df = [Math.floor(120*cycle_f), 120];;
    } else {
        let at = pw;
        let ct = Math.ceil(at/cycle_f);
        at = Math.round(ct * cycle_f);
        
        df = [at, ct];
    }
    if (df[0] < 5) {
        df[1] = Math.round(5 / cycle_f);
        df[0] = 5;
    }
    if (df[1] > 256) {
        df[0] = Math.round(256 * cycle_f);
        df[1] = 256;
    }    
    const fueling = df[0];
    const burnout = df[1]-df[0];
    fueling_el.value = fueling.toFixed(0);
    burnout_el.value = burnout.toFixed(0);
}

function params_to_power(vyhrevnost_el, f1kgt_el, power_el, fueling_el, burnout_el) {
    let ct = (fueling_el.valueAsNumber + burnout_el.valueAsNumber)
    let pw = calculate_power(vyhrevnost_el.valueAsNumber,
                    f1kgt_el.valueAsNumber, ct, fueling_el.valueAsNumber); 
    console.log(pw);
   power_el.value = Math.round(pw).toFixed(0);
}

function power_conv_init(el) {
    const lst = el.getElementsByTagName("input");
    let controls = {};
    Array.prototype.forEach.call(lst, x => controls[x.name] = x);
    let hh = [];
    let heat_value = controls["hval"];
    let f1kgt = controls["tray.f1kgt"];
    ["full", "low"].forEach(pfx => {
        let power_value = controls[pfx + ".power_value"];
        let fueling = controls[pfx + ".fueling"];
        let burnout = controls[pfx + ".burnout"];
        let p2w = params_to_power.bind(this, heat_value, f1kgt, power_value, fueling, burnout);
        let w2p = power_to_params.bind(this, heat_value, f1kgt, power_value, fueling, burnout, pfx == "low");        
        [fueling, burnout].forEach(x => x.onchange = x.oninput = p2w); 
        power_value.onchange = w2p;
        power_value.oninput = w2p;
        hh.push(w2p);
        p2w();
    });
    
    [heat_value, f1kgt].forEach(x=>x.onchange = x.oninput = () => {
        hh.forEach(x => x());
    });
    el.querySelector("[name=measure_speed]").onclick=function() {
        let win = ids["mereni_okno"];
        win.hidden = false;        
    };
    ids["usemeasuredvalue"].onclick = function(){
        f1kgt.valueAsNumber = ids["measurefeederspeed"].valueAsNumber;
        let win2 = ids["mereni_okno"];
        win2.hidden = true;
        hh.forEach(x => x());
    }
}

function dialog_nastaveni_zasobniku() {
    let win = ids["nastav_zasobnik"];
    hide_error(win);
    win.hidden = false;
    let inputs = {};
    Array.prototype.forEach.call(win.getElementsByTagName("input"), x => {
        inputs[x.name] = x;
        x.checked = false;
        x.value = "0";
    });
    inputs.hmotnost_pytle.value=Controller.config.bgkg;
    inputs.kapacita_zas.value=Controller.config.traykg;
    let buttons = win.getElementsByTagName("button");
    function endisbut(y) {
        Array.prototype.forEach.call(buttons, x => x.disabled = y);
    }

    async function do_req(full) {
        const kalib = inputs.kalib.checked;
        const absnow = inputs.absnow.checked;
        const bagcount = inputs.pytle.valueAsNumber;
        const mb = inputs.hmotnost_pytle.valueAsNumber;
        const kap = inputs.kapacita_zas.valueAsNumber;
        const kgchg = bagcount * mb;
        const limit = parseFloat(Controller.config.traykg); 
        if (!full) {
            hide_error(win);
            if (isNaN(kgchg)) return show_error(win, "prazdne");
            if (kgchg < -limit) return show_error(win, "male");
            if (kgchg > limit) return show_error(win, "velke");
        }
        if (mb < 1 || mb > kap) {
            return show_error(win, "neplatnahodnotahp");
        }
        if (kap < 1 || kap > 255) {
            return show_error(win, "neplatnahodnotakap");
        }
        try {
            let req = { absnow: absnow ? 1 : 0, kalib: kalib ? 1 : 0 };
            if (full) req["full"] = 1; else req["kgchg"] = kgchg;
            endisbut(true);
            let p1 = connection.send_request("f", encodeBinaryFrame(SetFuelWs, req));;
            let p2 = Controller.set_config({"bgkg":mb,"traykg":kap});
            let rr = await Promise.all([p1,p2]);
            let r = rr[0];
            let w = new Uint8Array(r);
            if (w.length == 0) {
                win.hidden = true;
                Controller.read_config();
            } else {
                show_error(win, "kalibselhal");
            }
            
        } catch (e) {
            show_error(win, "spojeni");
        }
        endisbut(false);
    }

    buttons[0].onclick = () => { do_req(true); };
    buttons[1].onclick = () => { do_req(false); };
}

function parseTextSector(buffer) {
    const uint8Array = new Uint8Array(buffer);
    let result = '';
    for (let i = 0; i < uint8Array.length; i++) {
        const byte = uint8Array[i];
        if (byte === 0) {
            break;
        }
        result += String.fromCharCode(byte);
    }
    return result;
}

function formatInterval(seconds) {
    if (!isFinite(seconds)) return "-";
    const sInMinute = 60;
    const sInHour = 60 * sInMinute;
    const sInDay = 24 * sInHour;
    const days = Math.floor(seconds / sInDay);
    seconds %= sInDay;
    const hours = Math.floor(seconds / sInHour);
    seconds %= sInHour;
    const minutes = Math.floor(seconds / sInMinute);
    seconds %= sInMinute;

    let result = '';
    if (days > 0) {
        result += `${days}d `;
    }
    result += `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}`;

    return result;
}

function dialog_registrace() {
    return new Promise((ok)=>{
        const replacements = {
            "0":"O","1":"I",
            "6":"G","7":"I",
            "2":"Z","3":"E",
            "5":"S","8":"B"
        };
        let win = ids["login"];
        let inputs =win.getElementsByTagName("input");
        let buttons = win.getElementsByTagName("button");
        Array.prototype.forEach.call(inputs ,(x,idx) =>{
            x.oninput =()=>{
                const v = replacements[x.value];
                if (v) x.value = v;
                x.value = x.value.toUpperCase();
                if (x.value < 'A' || x.value>'Z') {
                    x.value = "";
                }
                if(x.value.length == 1) {
                    if (idx < inputs.length-1) {
                        inputs[idx+1].focus();
                    } else {
                        buttons[1].focus();
                    }
                }
            };
            x.onkeydown = (ev)=>{
                if (ev.key == "Backspace" && x.value == "" && idx > 0) {
                    inputs[idx-1].focus();
                 }
            }
            x.value = "";
        });  
        win.hidden = false; 
        inputs[0].focus();
        buttons[0].onclick = async ()=>{
            hide_error(win);
            try {
                let resp = await fetch("/api/code", {
                    "method":"POST"
                });
                inputs[0].focus();
            } catch (e) {
                show_error(win,"spojeni");
            }
        }
        buttons[1].onclick = async () =>{
            hide_error(win);
            let code = inputs[0].value+inputs[1].value+inputs[2].value+inputs[3].value;
            if (code.length < 4) show_error(win,"chybipismena");
            else {
                try {
                    let resp = await fetch("/api/code",{
                        method: "POST",
                        body: code
                    })
                    if (resp.status != 200) {
                        show_error(win,"neplatny");
                    } else {
                        let r = parse_response(await resp.text());
                        ok(r.token);                                                 
                        win.hidden = true;
                    }                
                } catch (e) {
                    show_error(win,"spojeni");
                }
            }
        }
    });
}

async function confirm_dlg(id) {
    return new Promise((ok,err)=>{
        let win = document.getElementById(id);
        win.hidden = false;
        let btns = win.getElementsByTagName("button");
        btns[0].onclick = ()=>{win.hidden = true; ok();};
        btns[1].onclick = ()=>{win.hidden = true; err();};
    });
}

async function  dialog_show_code() {
    let win = ids["showcode"];
    win.hidden = false;
    let tds = win.getElementsByTagName("td");
    Array.prototype.forEach.call(tds,x=>x.textContent = "•");
    let btns = win.getElementsByTagName("button");
    btns[2].disabled = false;
    btns[2].onclick = async ()=>{
        let code =await connection.send_request("G");
        Array.prototype.forEach.call(tds,(x,idx)=>x.textContent = code[idx]);    
        btns[2].disabled = true;
    };
    btns[1].onclick = async ()=>{
        win.hidden = true;
        await confirm_dlg("potvrd_odparovani");
        let tkn = parse_response(await connection.send_request("U"));
        connection.set_token(tkn.token);
    };
    btns[0].onclick = async ()=>{
        win.hidden = true;
        await confirm_dlg("potvrd_odhlaseni");
        connection.set_token("");
        connection.reconnect("stop");
    };
    
}

async function vynulovat_statistiky() {
    async function doit() {
        try {
            await confirm_dlg("potvrd_vynulovat");
            while (true) {
                try {
                    await connection.send_request('0');            
                    Controller.update_stats_cycle();
                    break;
                } catch (e) {
                    console.error(e);
                }        
            }
        } catch (e) {}
        
    }
    this.disabled = true;
    await doit();
    this.disabled = false;
}

function updateFeederMeasure() {
    const a = ids["measurefeedertime"].valueAsNumber;
    const b = ids["measurefillup"].valueAsNumber;
    localStorage["measure_feeder_fillup"] = b;
    ids["measurefeederspeed"].valueAsNumber = Math.round(calc_feeder_speed(a,b));
}

async function main() {
    let ignore_man_change = 0;

    Controller.on_status_update = function(st) {
        if (!ignore_man_change) {
            if (st.fan == 0) delete Controller.man.fan;
            else ids["man_fan_speed"].value = st.fan;
            if (st.feeder == 0) delete Controller.man.feeder;
            if (st.mode == 1) Controller.config.m = 0;
            if (st.mode == 2) Controller.config.m = 1;
            ids["man_feeder"].classList.toggle("active", st.feeder != 0);
            ids["man_fan"].classList.toggle("active", st.fan != 0);
        } else {
            --ignore_man_change;
        }
        let stav = ids["stav"];
        stav.className = "mode" + st.mode + " " + "automode" + st.automode + " " + "op" + Controller.config.m;
        update_temperature("vystupni_teplota", st["temp_output_value"],
            Controller.config["tout"], st["temp_output_amp_value"]);
        update_temperature("vstupni_teplota", st["temp_input_value"],
            Controller.config["tin"], st["temp_input_amp_value"]);
        update_fuel("zasobnik", calculate_fuel_remain());
        ids["ovladac_feeder"].classList.toggle("on", st.feeder != 0);
        ids["ovladac_fan"].classList.toggle("on", st.fan != 0);
        ids["ovladac_pump"].classList.toggle("on", st.pump != 0);
        ids["rssi"].textContent = st["rssi"];
        ids["netstatus"].classList.remove("neterror");
        ids["mototempmax"].hidden = st["feeder_overheat"] == 0;
        ids["manualcontrolpanel"].hidden = st.mode != 1;
        ids["zasobnik"].classList.toggle("open", st.tray_open != 0);
        ids["simul_temp"].hidden = st["temp_sim"] == 0;
        ids["devicetime"].textContent = st["time"].toLocaleTimeString();
        ids["devicedate"].textContent = st["time"].toLocaleDateString();
    };
    Controller.on_error = function(x, y) {
        ids["netstatus"].classList.add("neterror");
        console.error(x, y);
    };
    ids["prepnout_rezim"].addEventListener("click", async function() {
        const new_mode = 1 - parseInt(Controller.config.m);
        ignore_man_change = true;
        this.disabled = true;
        await Controller.set_config({ "m": new_mode });
        update_config_form(Controller.config);
        this.disabled = false;
        Controller.update_status_cycle();
    });

    ids["zasobnik"].parentNode.addEventListener("click", function() {
        let el = ids["nastav_zasobnik"];
        dialog_nastaveni_zasobniku();
    });
    ids["vystupni_teplota"].parentNode.addEventListener("click", function() {
        dialog_nastaveni_teploty("tout", "tsoutaddr", "touts");
    });
    ids["vstupni_teplota"].parentNode.addEventListener("click", function() {
        dialog_nastaveni_teploty("tin", "tsinaddr", "tins");
    });
    ids["horeni"].addEventListener("click", function() {
        nastav_horeni();
    });
    ids["wifi"].addEventListener("click", function() {
        nastav_wifi();
    });
    ids["man_feeder"].addEventListener("click", function() {
        ignore_man_change = 3;
        Controller.man.feeder = Controller.status.feeder == 0;
        this.classList.toggle("active");

    });
    ids["man_fan"].addEventListener("click", function() {
        ignore_man_change = 3;
        Controller.man.fan = Controller.status.fan == 0;
        this.classList.toggle("active");
    });
    let el = ids["man_fan_speed"];
    el.addEventListener("change", function() {
        ignore_man_change = 3;
        Controller.man.fan_speed = this.value;
    });
    el.value = 100;
    ids["pump_active_forever"].addEventListener("change", function() {
        Controller.man.force_pump = this.checked;

    });
    ids["stats_win"].addEventListener("click", function(){
        ids["stats"].hidden = false;
    });    
    ids["stats"].querySelector("button")
        .addEventListener("click", vynulovat_statistiky);
    el = ids["measurefillup"];
    el.addEventListener("input", updateFeederMeasure.bind(this));
    el.value = localStorage["measure_feeder_fillup"]; 
    ids["resetmeasurefeeder"].addEventListener("click", ()=>{
        localStorage["measure_feeder_start"] = Controller.stats["feeder_time"];
        ids["measurefeedertime"].valueAsNumber = 0;
        updateFeederMeasure();
    });
    
    
    const day_seconds = 24*60*60;
    
        Controller.on_stats_update = (data) => {
        const stattbl = ids["stats"];
        data["active_avg"] = data["active_time"]/data["start_count"];
        data["auto_time"] = data["full_power_time"]+data["low_power_time"]+data["cooling_time"];
        data["switch_mode_count"] = data["full_power_count"]+data["low_power_count"]+data["cool_count"];
        data["full_power_avg"] = data["full_power_time"]/data["full_power_count"];
        data["low_power_avg"] = data["low_power_time"]/data["low_power_count"];
        data["cooling_avg"] = data["cooling_time"]/data["cool_count"];
        data["auto_avg"] = data["auto_time"]/data["switch_mode_count"];
        data["stop_avg"] = data["stop_time"]/data["stop_count"];
        data["overheat_avg"] = data["overheat_time"]/data["overheat_count"];
        data["pump_avg"] = data["pump_time"]/data["pump_start_count"];
        data["fan_avg"] = data["fan_time"]/data["fan_start_count"];
        data["feeder_avg"] = data["feeder_time"]/data["feeder_start_count"];
        data["consumed_kg_avg"] = data["consumed_kg_total"]/(data["active_time"]/day_seconds);
        data["other_failure_count"] = data["stop_count"] - data["overheat_count"] - data["feeder_overheat_count"] -data["temp_read_failure_count"];        

        Array.prototype.forEach.call(stattbl.getElementsByTagName("td"),(el)=>{
             if (el.dataset.name) {
                let s = "";
                const val = data[el.dataset.name];
                const format = el.dataset.type;
                 if (isNaN(val)){
                     el.textContent = "-";
                } else if (format == "time") {
                    el.textContent = formatInterval(val);
                } else if (format == "count") {
                    el.textContent = val + "x";
                } else if (format) {
                    el.textContent = val.toFixed(0) + " "+format;
                } else {
                    el.textContent = val.toFixed(0);
                }
             }
        });

        let zero_timer = localStorage["measure_feeder_start"];
        zero_timer = zero_timer?parseInt(zero_timer):0;
        ids["measurefeedertime"].valueAsNumber = data["feeder_time"] - zero_timer;
        updateFeederMeasure();                
    }

    
    connection.onconnect = async function() {
        Controller.read_config();
        let data = await connection.send_request(6, {});
        ids["ssid"].textContent = parseTextSector(data);
        Controller.update_stats_cycle();
        Controller.update_status_cycle();
    };

    connection.ontokenreq = dialog_registrace;

    connection.connect();

}


