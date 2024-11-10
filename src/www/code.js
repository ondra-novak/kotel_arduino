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
            let resp = await fetch("/api/manual_control", {
                headers:{"Content-Type":"application/x-www-form-urlencoded"},
                method: "POST",
                body: convert_to_form_urlencode(req)                
            });            
            this.status = parse_response(await resp.text());
            this.on_status_update(this.status);
        } catch (e) {
            console.error(e);
        }
        setTimeout(this.update_status_cycle.bind(this), 1000); 
    },
    
    update_stats_cycle: async function() {
        try {
            let resp = await fetch("/api/stats");            
            this.state = parse_response(await resp.text());
            this.on_stats_update(this.state);
        } catch (e) {
            console.error(e);
        }
        setTimeout(this.update_stats_cycle.bind(this), 60000);         
    },
    
    read_config: async function() {
        while (true) {        
            try {
                this.config =  parse_response(await(await fetch("/api/config")).text());
                return this.config;
            } catch (e) {
                console.error(e);
            }
            await delay(1000);
        }
    },
    
    on_status_update: function (x) {console.log("status",x);},
    on_stats_update: function (x) {console.log("stats",x);},

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
        document.getElementById("ovladac_feeder").classList.toggle("on", st.feeder != "0");
        document.getElementById("ovladac_fan").classList.toggle("on", st.fan != "0");
        document.getElementById("ovladac_pump").classList.toggle("on", st.pump != "0");
    }
    document.getElementById("prepnout_rezim").addEventListener("click",async function() {
        const new_mode = 1-parseInt(Controller.config.operation_mode);
        this.disabled = true;
        await Controller.set_config({"operation_mode":new_mode});
        update_config_form(Controller.config);
        this.disabled = false; 
    });
    
}

 
