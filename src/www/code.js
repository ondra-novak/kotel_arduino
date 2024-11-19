"use strict";

function convert_to_form_urlencode(req) {
    return Object.keys(req).map(x=>{
        return encodeURIComponent(x) + "=" + encodeURIComponent(req[x]);
    }).join('&');
}


function delay(millis) {
    return new Promise(ok=>{
        setTimeout(ok, millis);
    });
}

function parse_response(text, sep="\r\n") {
    return text.split(sep).reduce((obj,line)=>{
        let kv = line.split('=',2).map(x=>x.trim());
        if (kv[0]) {
            obj[kv[0]] = kv[1];
        }
        return obj;
    },{});
}


let Controller = {
    man: {
/*        force_pump: false,
        feeder: false,
        fan: false,
        fan_speed: null,*/
    },
    
    status:{},
    config:{},
    stats:{},
        update_status_cycle: async function() {
        if (this._status_timer) clearTimeout(this._status_timer);
        let req = {};
        if (this.man.feeder !== undefined) {
            req["feeder.timer"] = this.man.feeder?3:0;
            if (!this.man.feeder) delete this.man.feeder;
        }
        if (this.man.fan !== undefined) {
            req["fan.timer"] = this.man.fan?3:0;
            if (!this.man.fan) delete this.man.fan;
        }
        if (this.man.fan_speed !==undefined) {
            req["fan.speed"] = this.man.fan_speed?this.man.fan_speed:0;
            delete this.man.fan_speed;
        }
        if (this.man.force_pump !==undefined) {
            req["pump.force"] = this.man.force_pump?1:0;
            delete this.man.force_pump;
        } 
        try {
            this.on_begin_refresh("status");
            let resp = await fetch("/api/manual_control", {
                headers:{"Content-Type":"application/x-www-form-urlencoded"},
                method: "POST",
                body: convert_to_form_urlencode(req),
                signal: AbortSignal.timeout(5000)                
            });            
            this.status = parse_response(await resp.text());
            this.on_status_update(this.status);
        } catch (e) {
            this.on_error("status",e);
        }
        this._status_timer = setTimeout(this.update_status_cycle.bind(this), 1000);
        this._man_copy = Object.assign({}, this.man); 
    },
    
    update_stats_cycle: async function() {
        if (this._stat_timer) killTimer(this._stat_timer);
        try {
            this.on_begin_refresh("stats");
            let resp = await fetch("/api/stats",{signal: AbortSignal.timeout(5000)});            
            this.stats = parse_response(await resp.text());
            this.on_stats_update(this.state);
        } catch (e) {
            this.on_error("stats",e);
        }
        this._stats_timer = setTimeout(this.update_stats_cycle.bind(this), 60000);         
    },
    
    read_config: async function() {
        while (true) {        
            try {
                this.on_begin_refresh("config");
                this.config =  parse_response(await(await fetch("/api/config")).text());
                return this.config;
            } catch (e) {
                this.on_error("config",e);
            }
            await delay(1000);
        }
    },
    
    on_status_update: function (x) {console.log("status",x);},
    on_stats_update: function (x) {console.log("stats",x);},
    on_error: function(x,y) {console.error(x,y);},
    on_begin_refresh: function() {},

    set_config: async function(config){
        if (Object.keys(config).length == 0) return;
        while (true) {
            try {
                let body = convert_to_form_urlencode(config);
                let resp = await fetch("/api/config", {
                    headers:{"Content-Type":"application/x-www-form-urlencoded"},
                    method: "PUT",
                    body: body,
                    signal: AbortSignal.timeout(5000)
                });
                if (resp.status == 202) {
                    let config_conv = parse_response(body,"&");
                    Object.assign(this.config, config_conv);
                    return true;
                } else {
                    console.error("Failed to store config", resp.status, body);
                    return false;
                }
                
            } catch (e) {
                console.error(e);
            }
            await delay(1000);
            
        }
    }
        
}

 function update_config_form(cfg) {     
    Object.keys(cfg).forEach(k=>{
        const v = cfg[k];
        document.getElementsByName(k).forEach(x=>{
            x.value=v;
        })        
    });
}


function update_temperature(id, temp, ref_temp, ampl_temp){
    const el = document.getElementById(id);
    const cur = el.getElementsByClassName("cur")[0];
    const ref = el.getElementsByClassName("ref")[0];
    const label = el.getElementsByClassName("cur_temp")[0];
    const ampl = el.getElementsByClassName("amp_temp")[0];
    
    temp = parseFloat(temp);
    ref_temp = parseFloat(ref_temp);
    ampl_temp = parseFloat(ampl_temp);
    
    
    function angle(temp) {        
        let a = (temp-20)*270/80;
        if (a < 0) a = 0;
        if (a > 270) a = 270;
        return (a - 45)+"deg";
    }
    
    cur.hidden = isNaN(temp);    
    cur.setAttribute("style","transform: rotate("+angle(temp)+");");
    ref.setAttribute("style","transform: rotate("+angle(ref_temp)+");");
    label.textContent = isNaN(temp)?"--.-":temp.toFixed(1);    
    ampl.textContent = ampl_temp.toFixed(1);
}

function update_fuel(id, value) {
    const el = document.getElementById(id);
    const pb = el.getElementsByClassName("fill")[0];
    const label = el.getElementsByClassName("label")[0];

    if (value>15) value = 15;
    const koef = 0.7;    
    const p = value / 15;
    const f = Math.floor((Math.sqrt(koef*p)+p*(1-Math.sqrt(koef)))*100);
    const fstr = f +"%";
    pb.setAttribute("style","height:"+fstr);
    label.textContent = value.toFixed(1);        
}

function calculate_fuel_remain() {
    const fill_time = parseFloat(Controller.stats["tray.fill_time"]);
    const cur_time = parseFloat(Controller.stats["feeder.time"]);
    const consup = parseFloat(Controller.config["tray.bag_consumption_time"]);
    const fillup = parseFloat(Controller.config["tray.bag_fill_count"]);
    
    const ellapsed = cur_time - fill_time;
    const consumed = ellapsed / consup;
    const remain = fillup < consumed?0:fillup - consumed;
    
    return remain;
    
    
}

function close_window() {
    this.parentElement.parentElement.hidden = true;
}

function show_error(win, error) {
    const lst =  win.getElementsByClassName("chyba");
    Array.prototype.forEach.call(lst,el=>{
        if (!error | el.classList.contains(error)) {
            el.classList.add("zobrazena");
        }
    });
}
function hide_error(win) {
    const lst =  win.getElementsByClassName("chyba");
    Array.prototype.forEach.call(lst,el=>{
        el.classList.remove("zobrazena");
    });
}    

async function nastav_zasobnik_ok() {
    const win = this.parentElement.parentElement;
    const kalib = win.querySelector("[name=kalib]").checked;   
    const absnow = win.querySelector("[name=absnow]").checked;
    const bagcount = win.querySelector("[name=pytle]").valueAsNumber;
    
    hide_error(win);
    if (isNaN(bagcount)) return show_error(win,"prazdne");
    if (bagcount < 0) return show_error(win,"zaporne");
    if (bagcount > 255) return show_error(win,"velke");
    
    console.log(kalib,absnow,bagcount);
    
    try {
        let resp = await fetch("/api/fuel", {
            method:"POST",
            body: convert_to_form_urlencode({
                bagcount:bagcount,
                absnow:absnow?1:0,
                kalib:kalib?1:0
            })
        });
        if (resp.status == 202) {
            close_window.call(this);
            Controller.read_config();               
        } else if (resp.status == 406){
            show_error(win,"kalibselhal");
        }
    } catch (e) {
        show_error(win,"spojeni");
    }
} 


function unhide_section() {
    this.parentElement.classList.toggle("unhidden");
}

async function nastav_teplomer(field) {
    let win = document.getElementById("nastav_hw_teplomer");
    let btn = win.getElementsByTagName("button"); 
    win.hidden = false;
    btn[0].onclick = ()=>{
        let cfg = {};
        cfg[field] = selected;
        btn[0].disabled = true;
        Controller.set_config(cfg);
        win.hidden = true;
    };
    btn[0].disabled = true;    
    let list = win.getElementsByClassName("list")[0];
    list.innerHTML="";
    let resp = await fetch("/api/scan_temp",{
        method:"post"
    });
    let selected = "";
    if (resp.ok) {
        resp = parse_response(await resp.text());
        Object.keys(resp).forEach(addr=>{
            let temp = resp[addr];
            let sp = document.createElement("span");
            let lb = document.createElement("label");            
            let elem = document.createElement("input");
            elem.setAttribute("type","radio");
            elem.setAttribute("name","t");
            elem.onchange = ()=>{
                selected=addr;
                btn[0].disabled = false;
            }   
            sp.appendChild(elem);
            sp.appendChild(document.createTextNode(temp));
            lb.appendChild(sp);
            list.appendChild(lb);
            lb.appendChild(document.createTextNode(addr));            
        });
    }
} 

function dialog_nastaveni_teploty(field, hw_field, trend_field) {
    let el = document.getElementById("nastav_teplotu");
    hide_error(el);
    el.hidden = false;
    el.dataset.field=field;
    el.dataset.hwfield=hw_field;
    let imp = el.getElementsByTagName("input")[1]
    let trnd = el.getElementsByTagName("input")[0];
    imp.value=Controller.config[field];
    trnd.value = parseFloat(Controller.config[trend_field]);
    let btm = el.getElementsByTagName("button");
    btm[0].onclick = ()=>{
        nastav_teplomer(hw_field);
    };
    btm[1].onclick = async ()=>{
        let cfg = {};
        let val = imp.valueAsNumber;
        let trnd_val = trnd.valueAsNumber;                
        if (isNaN(val)) {
            show_error(el,"prazdne")
        } else if (val < 30) {
            show_error(el,"male")
        } else if (val > 90) {
            show_error(el,"velke")
        } else if (isNaN(trnd_val)) {
            show_error(el,"trnd_prazdne")
        } else if (trnd_val < 0) {
            show_error(el,"trnd_male")
        } else if (trnd_val > 25) {
            show_error(el,"trnd_velke")        
        } else {
            cfg[field] = val;
            cfg[trend_field]=(trnd_val).toFixed(0);
            btm[1].disabled = true;
            await Controller.set_config(cfg);
            el.hidden = true;
        }
    }
    btm[1].disabled = false;
}

async function nastav_parametry(id) {
    let win = document.getElementById(id);
    hide_error(win);
    win.hidden = false;
    let inputs = win.getElementsByTagName("input"); 
    let buttons = win.getElementsByTagName("button"); 
    Array.prototype.forEach.call(inputs,x=>{
        x.value = Controller.config[x.name]; 
    });
    buttons[0].onclick= async ()=>{
        let cfg = {};
        let err = false; 
        Array.prototype.forEach.call(inputs,x=>{
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
            show_error(win,"fail");
        } else {
            buttons[0].disabled = true;
            await Controller.set_config(cfg);
            buttons[0].disabled = false;        
            win.hidden = true;
        }    
    }
}

async function nastav_wifi() {
    let win = document.getElementById("nastav_wifi");
    hide_error(win);
    win.hidden = false;
    let inputs = win.getElementsByTagName("input"); 
    let buttons = win.getElementsByTagName("button"); 
    Array.prototype.forEach.call(inputs,x=>{
        x.value = Controller.config[x.name] || ""; 
    });
    let current_password = Controller.config["wifi.password"]; 
    inputs[0].checked = Controller.config["net.ip"] == "0.0.0.0";
    buttons[0].onclick= async ()=>{
        let cfg = {};
        let err = false; 
        Array.prototype.forEach.call(inputs,x=>{
            if (x.getAttribute("type") == "text") {
                let val = x.value
                let name = x.name;                
                if (name.startsWith("net.")) {
                    const fld = val.split('.');
                    const e = fld.find(x=>{
                        const v = parseInt(x);
                        return isNaN(v) || v < 0 || v > 255;
                    })
                    if (e !== undefined) err = true;
                }
                cfg[name] = val;
            }        
        });
        if (err) {
            show_error(win,"addrerror");
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
} 


function power_to_params(vyhrevnost_el, power_el, fueling_el, burnout_el) {
    const ref_power = 70*vyhrevnost_el.valueAsNumber/17.0;
    const ref_power10 = Math.floor(ref_power * 10);
    const need_power10 = Math.floor(power_el.value*10);
    let a = 1;
    let b = 20;    
    while (b < 60) {
        a = b / (ref_power10/need_power10);
        if (a >4  && b - a >= 20 && a == Math.round(a)) break;
        b = b + 1;
    }
    if (b < 60) {
        fueling_el.value = a;
        burnout_el.value = b - a;
        return;        
    }
    const fueling = Math.max(2.0,Math.floor(power_el.valueAsNumber*(30+power_el.valueAsNumber)/ref_power));
    const cycle = Math.round(fueling*ref_power/power_el.valueAsNumber);
    const burnout = cycle - fueling;
    fueling_el.value = fueling.toFixed(0);
    burnout_el.value = burnout.toFixed(0);
}

function params_to_power(vyhrevnost_el, power_el, fueling_el, burnout_el) {
    const ref_power = 70*vyhrevnost_el.valueAsNumber/17.0;
    power_el.value = (fueling_el.valueAsNumber * ref_power / (fueling_el.valueAsNumber + burnout_el.valueAsNumber)).toFixed(1);   
}

function power_conv_init(el) {
    const lst = el.getElementsByTagName("input");
    let controls = {};
    Array.prototype.forEach.call(lst, x=>controls[x.name] = x);
    let hh = [];
    ["full","low"].forEach(pfx=>{
        let power_value = controls[pfx+".power_value"];
        let heat_value = controls["heat_value"];
        let fueling = controls[pfx+".fueling"];
        let burnout = controls[pfx+".burnout"];
        let p2w = params_to_power.bind(this,heat_value,power_value,fueling, burnout);
        let w2p = power_to_params.bind(this,heat_value,power_value,fueling, burnout);
        [fueling, burnout].forEach(x=>x.onchange=p2w);
        [fueling, burnout].forEach(x=>x.oninput=p2w);
        power_value.onchange=w2p;
        power_value.oninput=w2p;
        hh.push(w2p);
        p2w();
    });    
    controls["heat_value"].onchange = ()=>{
        hh.forEach(x=>x());
    };
}

function dialog_nastaveni_zasobniku() {
    let win = document.getElementById("nastav_zasobnik");
    hide_error(win);
    win.hidden = false;
    let inputs = {};
    Array.prototype.forEach.call(win.getElementsByTagName("input"),x=>{
        inputs[x.name] = x;
        x.checked = false;
        x.value="0";
    }); 
    let buttons = win.getElementsByTagName("button"); 

    async function do_req(full) {
        const kalib = inputs.kalib.checked;   
        const absnow = inputs.absnow.checked;
        const bagcount = inputs.pytle.valueAsNumber;
        if (!full) {
            hide_error(win);
            if (isNaN(bagcount)) return show_error(win,"prazdne");
            if (bagcount < -15) return show_error(win,"male");
            if (bagcount > 15) return show_error(win,"velke");
        }
        try {
            let req = {absnow:absnow?1:0, kalib:kalib?1:0};
            if (full) req["full"] = 1; else req["bagcount"] = bagcount;            
            let resp = await fetch("/api/fuel", {
                method:"POST",
                body: convert_to_form_urlencode(req)
            });
            if (resp.status == 202) {
                win.hidden = true;
                Controller.read_config();               
            } else if (resp.status == 406){
                show_error(win,"kalibselhal");
            }            
        } catch (e) {
            show_error(win,"spojeni");
        }
        
    }
    
    buttons[0].onclick = ()=>{do_req(true);};
    buttons[1].onclick = ()=>{do_req(false);};        
}

async function main() {
    
    
    Controller.on_status_update = function(st) {
        let stav = document.getElementById("stav");
        stav.className="mode"+st.mode+" "+"automode"+st.auto_mode;
        update_temperature("vystupni_teplota", st["temp.output.value"], 
                        Controller.config["temperature.max_output"], st["temp.output.ampl"]);
        update_temperature("vstupni_teplota", st["temp.input.value"], 
                        Controller.config["temperature.min_input"], st["temp.input.ampl"]);
        update_fuel("zasobnik",calculate_fuel_remain()); 
        document.getElementById("ovladac_feeder").classList.toggle("on", st.feeder != "0");
        document.getElementById("ovladac_fan").classList.toggle("on", st.fan != "0");
        document.getElementById("ovladac_pump").classList.toggle("on", st.pump != "0");
        document.getElementById("ssid").textContent = st["network.ssid"];
        document.getElementById("rssi").textContent = st["network.signal"];
        document.getElementById("netstatus").classList.remove("netconnecting");
        document.getElementById("mototempmax").hidden = st["motor_temp_ok"] == "1";
        document.getElementById("manualcontrolpanel").hidden = st.mode != "1";
        document.getElementById("man_feeder").classList.toggle("active",st.feeder != "0");
        document.getElementById("man_fan").classList.toggle("active",st.fan != "0");
        if (st.fan != "0" && st.mode != "1") document.getElementById("man_fan_speed").value=st.fan;
        if (st.mode != '1') {
            Controller.man.fan = 0;
            Controller.man.feeder = 0;
        }
        document.getElementById("simul_temp").hidden = st["temp.sim"] == '0';
    };
    Controller.on_error = function(x,y) {
        document.getElementById("netstatus").classList.add("neterror");
        document.getElementById("netstatus").classList.remove("netconnecting");
        console.error(x,y);
    };
    Controller.on_begin_refresh = function(x) {
        document.getElementById("netstatus").classList.remove("neterror");
        document.getElementById("netstatus").classList.add("netconnecting");
    };
    document.getElementById("prepnout_rezim").addEventListener("click",async function() {
        const new_mode = 1-parseInt(Controller.config.operation_mode);
        this.disabled = true;
        await Controller.set_config({"operation_mode":new_mode});
        update_config_form(Controller.config);
        this.disabled = false; 
        Controller.update_status_cycle();
    });

    let el = document.getElementById("zasobnik").parentNode;
    el.addEventListener("click",function(){
        let el = document.getElementById("nastav_zasobnik");
        dialog_nastaveni_zasobniku();
    });
    el = document.getElementById("vystupni_teplota").parentNode;
    el.addEventListener("click",function(){
        dialog_nastaveni_teploty("temperature.max_output","temp_sensor.output.addr","temperature.max_output_samples");
        
    });
    el = document.getElementById("vstupni_teplota").parentNode;
    el.addEventListener("click",function(){
        dialog_nastaveni_teploty("temperature.min_input","temp_sensor.input.addr","temperature.min_input_samples");        
    });
    el = document.getElementById("horeni");
    el.addEventListener("click", function(){
        nastav_parametry("nastav_topeni");
        power_conv_init(document.getElementById("nastav_topeni"));
    });
    el = document.getElementById("wifi");
    el.addEventListener("click", function(){
        nastav_wifi();
    });
    el = document.getElementById("man_feeder");
    el.addEventListener("click", function() {
        Controller.man.feeder = Controller.status.feeder == "0";
        Controller.update_status_cycle();
    });
    el = document.getElementById("man_fan");
    el.addEventListener("click", function() {
        Controller.man.fan = Controller.status.fan == "0";
        Controller.update_status_cycle();
    });
    el = document.getElementById("man_fan_speed");    
    el.addEventListener("change",function(){
        Controller.man.fan_speed = this.value; 
    });
    el.value = 100;
    el = document.getElementById("pump_active_forever");
    el.addEventListener("change",function(){
        Controller.man.force_pump = this.checked;
        
    })

    Controller.update_status_cycle();
    Controller.update_stats_cycle();
    await Controller.read_config();

}

 
