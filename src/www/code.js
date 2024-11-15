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

function parse_response(text) {
    return text.split("\r\n").reduce((obj,line)=>{
        let kv = line.split('=',2).map(x=>x.trim());
        if (kv[0]) {
            obj[kv[0]] = kv[1];
        }
        return obj;
    },{});
}


let Controller = {
    man: {
        force_pump: false,
        feeder: false,
        fan: false,
        fan_speed: null,
    },
    
    status:{},
    config:{},
    stats:{},
    update_status_cycle: async function() {
        
        let req = {
            "feeder.timer": this.man.feeder?3:0,
            "fan.timer": this.man.fan?3:0,
            "pump.force": this.man.force_pump?1:0,
            "fan.speed": this.man.fan_speed?this.man.fan_speed:0
        }       
        try {
            this.on_begin_refresh("status");
            let resp = await fetch("/api/manual_control", {
                headers:{"Content-Type":"application/x-www-form-urlencoded"},
                method: "POST",
                body: convert_to_form_urlencode(req)                
            });            
            this.status = parse_response(await resp.text());
            this.on_status_update(this.status);
        } catch (e) {
            this.on_error("status",e);
        }
        setTimeout(this.update_status_cycle.bind(this), 1000); 
    },
    
    update_stats_cycle: async function() {
        if (this._stat_timer) killTimer(this._stat_timer);
        try {
            this.on_begin_refresh("stats");
            let resp = await fetch("/api/stats");            
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
                    body: body
                });
                if (resp.status == 202) {
                    let config_conv = parse_response(body);
                    Object.assign(this.config, config_conv);
                    return true;
                } else {
                    console.error("Failed to store config", resp.status, body);
                    return false;
                }
                
            } catch (e) {
                console.error(e);
            }
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

function update_temperature(id, temp, ref_temp){
    const el = document.getElementById(id);
    const cur = el.getElementsByClassName("cur")[0];
    const ref = el.getElementsByClassName("ref")[0];
    const label = el.getElementsByClassName("cur_temp")[0];
    
    temp = parseFloat(temp);
    ref_temp = parseFloat(ref_temp);
    
    function angle(temp) {
        const a = (temp-20)*270/80;
        if (a < 0) a = 0;
        if (a > 270) a = 270;
        return (a - 45)+"deg";
    }
    
    cur.setAttribute("style","transform: rotate("+angle(temp)+");");
    ref.setAttribute("style","transform: rotate("+angle(ref_temp)+");");
    label.textContent = temp.toFixed(1);    
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
    win.hidden = false;
    win.getElementsByTagName("button")[0].onclick = ()=>{
        
    };
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
            let lb = document.createElement("label");            
            let elem = document.createElement("input");
            elem.setAttribute("type","radio");
            elem.setAttribute("name","t");
            elem.onselect = ()=>selected=addr;   
            lb.appendChild(elem);
            list.appendChild(lb);
            lb.appendChild(document.createTextNode(addr + ": " + temp));            
        });
    }
} 

function main() {
    Controller.update_status_cycle();
    Controller.update_stats_cycle();
    Controller.read_config().then(update_config_form);
    
//    var stringtable = document.getElementById("stringtable");
    
    Controller.on_status_update = function(st) {
        let stav = document.getElementById("stav");
        stav.className="mode"+st.mode+" "+"automode"+st.auto_mode;
        update_temperature("vystupni_teplota", st["temp.output.value"], Controller.config["temperature.max_output"]);
        update_temperature("vstupni_teplota", st["temp.input.value"], Controller.config["temperature.min_input"]);
        update_fuel("zasobnik",calculate_fuel_remain()); 
        document.getElementById("ovladac_feeder").classList.toggle("on", st.feeder != "0");
        document.getElementById("ovladac_fan").classList.toggle("on", st.fan != "0");
        document.getElementById("ovladac_pump").classList.toggle("on", st.pump != "0");
        document.getElementById("netstatus").classList.remove("netconnecting");
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
    });

    let el = document.getElementById("zasobnik").parentNode;
    el.addEventListener("click",function(){
        let el = document.getElementById("nastav_zasobnik");
        el.hidden = false;
        el.getElementsByTagName("input")[0].value=15;  
    });
    el = document.getElementById("vystupni_teplota").parentNode;
    el.addEventListener("click",function(){
        let el = document.getElementById("nastav_teplotu");
        el.hidden = false;
        el.dataset.typ="vystupni";
        let imp = el.getElementsByTagName("input")[0]
        imp.dataset.name="temperature.max_output";
        imp.value=Controller.config[imp.dataset.name];
        
    });
    el = document.getElementById("vstupni_teplota").parentNode;
    el.addEventListener("click",function(){
        let el = document.getElementById("nastav_teplotu");
        el.hidden = false;
        el.dataset.typ="vstupni";
        let imp = el.getElementsByTagName("input")[0]
        imp.dataset.name="temperature.min_input";
        imp.value=Controller.config[imp.dataset.name];
        
    });
    
}

 
