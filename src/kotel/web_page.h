#pragma once

#include <string_view>

/*
template<typename ... Fn>
class LongString: public std::string_view {
public:
    static constexpr std::size_t total_sz = (0+...+Fn()().size());

    constexpr LongString(Fn ... fns):std::string_view(data, total_sz) {
        char *iter = data;
        auto write = [&](char * &iter, auto x) {
            iter = std::copy(x.begin(), x.end(), iter);
        };
        (..., write(iter, fns()));
    }
protected:
    char data[total_sz] = {};

};

constexpr LongString client_embedded_js(
        []()->std::string_view{return R"javascript()javascript";},
        //split at 16KB
        []()->std::string_view{return R"javascript()javascript";});
*/

template<unsigned int N>
constexpr std::string_view string_constant(const char (&str)[N]) {
    return std::string_view(str, N-1);
} 

constexpr auto embedded_index_html = string_constant(R"###(<!DOCTYPE html>
<html>

<head>
    <title>Kotel</title>
    <link rel="stylesheet" href="style.css">
    <meta name="viewport" content="width=device-width, initial-scale=1" />
</head>

<body onload="main()">
    <script src="code.js" type="text/javascript"></script>
    <span id="stringtable" data-cur_mode0="Neznámý stav" data-cur_mode1="Ručně" data-cur_mode2="Automaticky"
        data-cur_mode3="STOP" data-cur_mode4="Ostatní" data-auto_mode0="Topení" data-auto_mode1="Útlum"
        data-auto_mode2="Obnova plamene"></span>
    <div class="header" id="netstatus">Kotel</div>
    <div id="stav" class="header_stav">
        <div class="button" id="prepnout_rezim"></div>
    </div>
    <div class="varovani" id="simul_temp" hidden="hidden">POZOR: Simulovaná teplota, pouze test!!!</div>
    <div class="dashboard">
        <div class="prvekspopisem">
            <span>Výstupní teplota</span>
            <div class="teplomer" id="vystupni_teplota">
                <div class="label t20">20</div>
                <div class="label t40">40</div>
                <div class="label t60">60</div>
                <div class="label t80">80</div>
                <div class="label t100">100</div>
                <div class="max ref needle"></div>
                <div class="cur needle"></div>
                <div class="label cur_temp">--.-</div>
                <div class="label amp_temp">--.-</div>
            </div>
        </div>
        <div class="prvekspopisem">
            <span>Vstupní teplota</span>
            <div class="teplomer" id="vstupni_teplota">
                <div class="label t20">20</div>
                <div class="label t40">40</div>
                <div class="label t60">60</div>
                <div class="label t80">80</div>
                <div class="label t100">100</div>
                <div class="min ref needle"></div>
                <div class="cur needle"></div>
                <div class="label cur_temp">--.-</div>
                <div class="label amp_temp">--.-</div>
            </div>
        </div>
        <div class="prvekspopisem">
            <span>Zásobník</span>
            <div class="zasobnik" id="zasobnik">
                <div class="fill" style="height: 0"></div>
                <div class="bok1 a"></div>
                <div class="bok1 b"></div>
                <div class="bok2 a"></div>
                <div class="bok2 b"></div>
                <div class="label">--</div>
            </div>
        </div>
        <div class="ovladace prvekspopisem" id="horeni">
            <span>Ovládání hoření</span>
            <div id="ovladac_feeder">Podavač<div class="icon">
                    <div class="iconfeeder"></div>
                </div>
                <div id="mototempmax" hidden="hidden"></div>
            </div>
            <div id="ovladac_fan">Ventilátor<div class="icon">
                    <div class="iconfan"></div>
                </div>
            </div>
            <div id="ovladac_pump">Čerpadlo<div class="icon">
                    <div class="iconpump"></div>
                </div>
            </div>
        </div>
        <div class="wifiinfo prvekspopisem" id="wifi">
            <span>Připojení </span>
            <div class="wifi"></div>
            <div class="label">
                <div id="ssid">---</div>
                <div id="rssi">---</div>
            </div>
        </div>
    <div class="prvekspopisem" id="stats_win">
        <span>Přehledy</span>
        <div class="wifi"></div>
        <div class="label">
            <div id="ssid">---</div>
            <div id="rssi">---</div>
        </div>
    </div>
    </div>
    <div id="manualcontrolpanel" hidden="hidden">
        <p>Panel ručního ovládání</p>
        <div class="buttons">
            <div class="button" id="man_feeder">Podavač<div class="iconfeeder"></div>
            </div>
            <div class="button" id="man_fan">Ventilátor<div class="iconfan"></div>
            </div>
        </div>
        <div class="slider">
            <input type="range" min="1" max="100" step="1" id="man_fan_speed">
        </div>
    </div>


    <div class="okno section" id="nastav_zasobnik" hidden="hidden">
        <div class="sectionname black">Přidat palivo</div>
        <div class="chyba spojeni">Chyba připojení!</div>
        <div class="sub_section unhide">
            <div class="sectionname" onclick="unhide_section.call(this);">Pokročilé volby</div>
            <label><span>Aktuální stav (ignoruj čas naplnění)</span><input type="checkbox" name="absnow"></label>
            <label><span>Kalibrovat (zásobník je prázdný)</span><input type="checkbox" name="kalib"></label>
            <div class="chyba kalibselhal">Kalibrace selhala, zásobník byl na začátku prázdný</div>
        </div>
        <label><span>Přidaný počet pytlů</span><input type="number" value="0" step="1" min="-15" max="15" size="3" name="pytle"></label>
        <div class="chyba male">Nesmí být záporné číslo</div>
        <div class="chyba velke">Moc velká hodnota</div>
        <div class="chyba prazdne">Musíš něco napsat</div>
        <div class="buttonpanel">
                <button>Plně naloženo</button>
                <button>OK</button> 
                <button onclick="close_window.call(this)">Storno</button></div>
    </div>

    <div class="okno section" id="nastav_teplotu" hidden="hidden">
        <div class="vstupni sectionname black">Minimální vstupní teplota</div>
        <div class="vystupni sectionname black">Maximální výstupní teplota</div>
        <div class="sub_section unhide">
            <div class="sectionname" onclick="unhide_section.call(this);">Pokročilé volby</div>
            <label><span>Síla trendu</span><input type="number" step="0.1" min="0" max="25"></label>
            <div class="chyba trnd_male">Moc malá hodnota</div>
            <div class="chyba trnd_velke">Moc velká hodnota</div>
            <div class="chyba trnd_prazdne">Musíš něco napsat</div>
            <label><span>HW adresa teploměru</span><button>Změnit</button></label>
        </div>
        <label><span>Teplota</span><input type="number" value="60" step="1" min="40" max="90" size="3"></label>
        <div class="chyba male">Moc malá teplota</div>
        <div class="chyba velke">Moc velká teplota</div>
        <div class="chyba prazdne">Musíš něco napsat</div>
        <div class="buttonpanel"><button onclick="nastav_teplotu_ok.call(this)">OK</button> <button
                onclick="close_window.call(this)">Storno</button></div>
    </div>

    <div class="okno section" id="nastav_hw_teplomer" hidden="hidden">
        <div class="sectionname black">Vyber teploměr</div>
        <div class="list">

        </div>
        <div class="buttonpanel"><button>OK</button> <button onclick="close_window.call(this)">Storno</button></div>

    </div>
    <div class="okno section" id="nastav_wifi" hidden="hidden">
        <div class="sectionname black">Nastavení připojení</div>
        <div class="sub_section unhide">
            <div class="sectionname" onclick="unhide_section.call(this);">Pokročilé volby</div>
            <label><span>Použít DHCP</span><input type="checkbox" name="dhcp"></label>
            <label><span>Adresa</span><input type="text" name="net.ip" maxlength="15" size="15"></label>
            <label><span>Maska adresy</span><input type="text" name="net.netmask" maxlength="15" size="15"></label>
            <label><span>Výchozí brána</span><input type="text" name="net.gateway" maxlength="15" size="15"></label>
            <label><span>DNS adresa</span><input type="text" name="net.dns" maxlength="15" size="15"></label>
            <div class="chyba addrerror">Některá z adres není v pořádku</div>
        </div>
        <label><span>Wifi SSID</span><input type="text" maxlength="20" size="17" name="wifi.ssid"></label>
        <label><span>Wifi heslo</span><input type="text" maxlength="20" size="17" name="wifi.password"></label>
        <p>Změny nastavení připojení se projeví po restartu zařízení</p>
        <div class="buttonpanel"><button>OK</button> <button onclick="close_window.call(this)">Storno</button></div>
    </div>
    <div class="okno section" id="nastav_topeni" hidden="hidden">
        <div class="sectionname black">Ovládání hoření</div>
        <label><span>Výhřevnost MJ/kg</span><input type="number" min="5" max="25.5" step="0.1"
                name="hval""></label>

        <div class="subsection">
            <div class="sectionname">Plný výkon</div>
            <label><span>Výkon kW</span><input type="number" min="1" max="25" step="0.5"
                    name="full.power_value" data-local="1"></label>
            <label><span>Ventilátor [%]</span><input type="number" min="1" max="100" step="1"
                    name="full.fanpw"></label>
        </div>
        <div class="subsection">
            <div class="sectionname">Snížený výkon</div>
            <label><span>Výkon kW</span><input type="number" min="0.5" max="25" step="0.5" 
                name="low.power_value" data-local="1"></label>
            <label><span>Výkon ventilátoru [%]</span><input type="number" min="1" max="100" step="1"
                    name="low.fanpw"></label>
        </div>
        <div class="subsection">
            <div class="sectionname">Čerpadlo</div>
            <label><span>Spínací výstupní teplota</span><input type="number" min="30" max="90" step="1"
                    name="tpump"></label>
            <div class="sub_section unhide">
                <div class="sectionname" onclick="unhide_section.call(this);">Pokročilé volby</div>
                <label><span>V ručním ovládání čerpadlo trvale zapnuté</span><input type="checkbox"
                        id="pump_active_forever"></label>
                        <label><span>Ventilátor freq.dělič</span><input type="number" min="1" max="255" step="1"
                                name="fanpc"></label>
                    <label><span>Plný výkon - přikládání [s]</span><input type="number" min="1" max="255" step="1"
                            name="full.fueling"></label>
                    <label><span>Plný výkon - prohořívání [s]</span><input type="number" min="1" max="255" step="1"
                            name="full.burnout"></label>
                    <label><span>Snížený výkon - přikládání [s]</span><input type="number" min="1" max="255" step="1"
                            name="low.fueling"></label>
                    <label><span>Snížený výkon - prohořívání [s]</span><input type="number" min="1" max="255" step="1"
                            name="low.burnout"></label>
            </div>
        </div>
        <div class="chyba fail">Některé hodnoty jsou chybně zadané nebo jsou mimo rozsah</div>
        <div class="buttonpanel"><button>OK</button> <button onclick="close_window.call(this)">Storno</button></div>
    </div>
</body>

</html>)###");
constexpr auto embedded_code_js = string_constant(R"###("use strict";

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

class WebSocketExchange {
    
    #ws = null;
    #tosend = [];
    #promises={};
    #enc = new TextEncoder();
    #pingtm = -1;
    onconnect = function(){};
    
    send_request(cmd, content) {
        return new Promise((ok,err) => {            
            if (typeof cmd == "string") cmd = this.#enc.encode(cmd);
            else if (typeof cmd == "number") cmd = Uint8Array.from([cmd]);
            else return Promise.reject(new TypeError("invalid command format"));
            if (typeof content == "string") content = this.#enc.encode(content);
            else if (!content instanceof ArrayBuffer) { 
                if (Array.isArray(content)) content = Uint8Array.from(content);
                else Promise.reject(new TypeError("invalid content format"));
            } else {
                content = new Uint8Array(content);
            }
            let selector = cmd[0];
            var mergedArray = new Uint8Array(cmd.length + content.length);
            mergedArray.set(cmd,0);
            mergedArray.set(content, cmd.length);
            if (!this.#promises[selector]) {
                this.#promises[selector] = [];
            }
            this.#promises[selector].push([ok,err]);
            this.#tosend.push(mergedArray);
            this.flush();
        });
    }
    
    reconnect(err) {
        if (this.#ws) {
            Object.keys(this.#promises).forEach(x=>this.#promises[x].forEach(z=>z[1](err)));
            this.#promises = {};
            this.#ws = null;
            clearTimeout(this.#pingtm);
            setTimeout(()=>this.flush(), 1000);
        }
    }

    reset_timeout() {
        if (this.#pingtm>=0) clearTimeout(this.#pingtm);
        this.#pingtm = setTimeout(()=>{
            this.#pingtm = -1;
            this.#ws.close();
            this.reconnect(new TypeError("connection timeout"));
        },5000);
    }
        
    flush() {
        if (!this.#ws) {
            this.#ws = new WebSocket(location.href.replace(/^http/,"ws")+"api/ws");
            this.#ws.binaryType = "arraybuffer";
            this.#ws.onerror = () => {this.reconnect(new TypeError("connection failed"));}
            this.#ws.onclose = () => {this.reconnect(new TypeError("connection reset"));}
            this.#ws.onmessage = (ev)=>{
                this.reset_timeout();
                let data = ev.data;
                let selector;
                if (typeof data == "string") {
                    selector = this.#enc.encode(data)[0];
                    data = data.substr(1);                
                } else {
                    data = new Uint8Array(data);
                    selector = data[0];
                    data = data.slice(1).buffer;
                    
                }
                let p = this.#promises[selector];
                if (p && p.length) {
                    let q = p.shift();
                    q[0](data);
                }
            }
            this.#ws.onopen = ()=>{
                this.onconnect();
                this.flush();
            }
        } else if (this.#ws.readyState == WebSocket.OPEN) {
            this.#tosend.forEach(x=>this.#ws.send(x));
            this.#tosend = [];
        }        
    }

    

    
}

const StatusOutWs = [
    ["uint32","feeder_time"],
    ["uint32","tray_open_time"],
    ["uint32","tray_fill_time"],
    ["int16","bag_fill_count"],
    ["int16","bag_consumption"],
    ["int16","temp_output_value"],
    ["int16","temp_output_amp_value"],
    ["int16","temp_input_value"],
    ["int16","temp_input_amp_value"],
    ["int16","rssi"],
    ["uint8","temp_sim"],
    ["uint8","temp_input_status"],
    ["uint8","temp_output_status"],
    ["uint8","mode"],
    ["uint8","automode"],
    ["uint8","tray_open"],
    ["uint8","motor_temp_ok"],
    ["uint8","pump"],
    ["uint8","feeder"],
    ["uint8","fan"],
];

const ManualControlWs = [
    ["uint8","feeder_timer"],
    ["uint8","fan_timer"],
    ["uint8","fan_speed"],
    ["uint8","force_pump"]  
];

const SetFuelWs = [
    ["int8","bagcount"],
    ["int8","kalib"],
    ["int8","absnow"],
    ["int8","full"],


]

function decodeBinaryFrame(pattern, buffer) {
    const view = new DataView(buffer);
    let out = {};
    let offset = 0;
    pattern.forEach(x=>{
        switch (x[0]) {
            case "uint32": out[x[1]] = view.getUint32(offset,true);offset+=4;break;
            case "int16": out[x[1]] = view.getInt16(offset,true);offset+=2;break;
            case "uint8": out[x[1]] = view.getUint8(offset,true);offset+=1;break;
            case "int8": out[x[1]] = view.getInt8(offset,true);offset+=1;break;
            default: throw new TypeError("unknown field type:" + x[0]);
        }
    })
    return out;
}


function encodeBinaryFrame(pattern,  data) {

    let arr = null;
    let view = null;
    for (let i = 0; i < 2; ++i) {
        let offset = 0;    
        pattern.forEach(x=>{
            switch(x[0]){
                case "uint32": if (view) view.setUInt32(offset, data[x[1]]); offset+=4;break;
                case "int16": if (view) view.setInt16(offset, data[x[1]]); offset+=2;break;
                case "uint8": if (view) view.setUint8(offset, data[x[1]]); offset+=1;break;
                case "int8": if (view) view.setInt8(offset, data[x[1]]); offset+=1;break;
                default: throw new TypeError("unknown field type:" + x[0]);
            }
        })    
        
        if (arr) break;
        arr = new ArrayBuffer(offset);
        view = new DataView(arr);        
    }
    return arr;
}

let connection = new WebSocketExchange();

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
    _promises:{},

    update_status_cycle: async function() {
        if (this._status_timer) clearTimeout(this._status_timer);
        try {
            let req = {};
            if (this.man.feeder !== undefined) {
                req.feeder_timer = this.man.feeder?3:0;
                if (!this.man.feeder) delete this.man.feeder;
            } else {
                req.feeder_timer = 255;
            }
            if (this.man.fan !== undefined) {
                req.fan_timer = this.man.fan?3:0;
                if (!this.man.fan) delete this.man.fan;
            } else {
                req.fan_timer = 255;
            }
                
            if (this.man.fan_speed !==undefined) {
                req.fan_speed = this.man.fan_speed?this.man.fan_speed:0;
                delete this.man.fan_speed;
            } else {
                req.fan_speed = 255;
            }
            if (this.man.force_pump !==undefined) {
                req.force_pump = this.man.force_pump?1:0;
                delete this.man.force_pump;
            } else {
                req.force_pump = 255;
            }
            let data = await connection.send_request("c", encodeBinaryFrame(ManualControlWs, req));
            let out = decodeBinaryFrame(StatusOutWs, data);
            if (out.temp_output_value < -10000) delete out.temp_output_value;
            else out.temp_output_value = out.temp_output_value*0.1;
            if (out.temp_input_value < -10000) delete out.temp_input_value;
            else out.temp_input_value = out.temp_input_value*0.1;
            out.temp_output_amp_value = out.temp_output_amp_value*0.1;
            out.temp_input_amp_value = out.temp_input_amp_value*0.1;
            this.status = out;
            this.on_status_update(out);        
        } catch (e) {
            this.on_error("status",e);
        }
        this._status_timer = setTimeout(this.update_status_cycle.bind(this), 1000);    
    },
    
    update_stats_cycle: async function() {
        if (this._stat_timer) killTimer(this._stat_timer);
        try {
            let resp = await connection.send_request("T",[]);
            this.stats = parse_response(resp);
            this.on_stats_update(this.stats);
        } catch (e) {
            this.on_error("stats",e);
        }
        this._stats_timer = setTimeout(this.update_stats_cycle.bind(this), 30000);         
    },
    
    read_config: async function() {
        while (true) {        
            try {
                this.config = parse_response(await connection.send_request('C',{}));
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
    
    set_config: async function(config){
        if (Object.keys(config).length == 0) return;
        while (true) {
            try {
                let body = convert_to_form_urlencode(config);
                let resp = await connection.send_request("S",body);
                let config_conv = parse_response(body,"&");
                Object.assign(this.config, config_conv);
                return true;
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
    const fill_time = parseFloat(Controller.stats["tray.ft"]);
    const cur_time = parseFloat(Controller.stats["feed.t"]);
    const consup = parseFloat(Controller.config["tray.bct"]);
    const fillup = parseFloat(Controller.config["tray.bgfc"]);
    
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
        let heat_value = controls["hval"];
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
    controls["hval"].onchange = ()=>{
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
    function endisbut(y) {
        Array.prototype.forEach.call(buttons, x=>x.disabled = y);
    }

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
            endisbut(true);
            let r = await connection.send_request("f",encodeBinaryFrame(SetFuelWs, req));
            let w = new Uint8Array(r);
            if (w.length==0) {
                win.hidden = true;
                Controller.read_config();               
            } else  {
                show_error(win,"kalibselhal");
            }            
        } catch (e) {
            show_error(win,"spojeni");
        }
        endisbut(false);
    }
    
    buttons[0].onclick = ()=>{do_req(true);};
    buttons[1].onclick = ()=>{do_req(false);};        
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

async function main() {
    let ignore_man_change = false;    
    
    Controller.on_status_update = function(st) {
        if (!ignore_man_change) {
            if (st.fan == 0)  delete Controller.man.fan;
            else document.getElementById("man_fan_speed").value=st.fan;
            if (st.feeder == 0) delete Controller.man.feeder;     
            if (st.mode == 1) Controller.config.m = 0;
            if (st.mode == 2) Controller.config.m = 1;
        } else {
            ignore_man_change = false;
        }
        let stav = document.getElementById("stav");
        stav.className="mode"+st.mode+" "+"automode"+st.automode+" "+"op"+Controller.config.m;
        update_temperature("vystupni_teplota", st["temp_output_value"], 
                        Controller.config["tout"],st["temp_output_amp_value"]);
        update_temperature("vstupni_teplota", st["temp_input_value"], 
                        Controller.config["tin"], st["temp_input_amp_value"]);
        update_fuel("zasobnik",calculate_fuel_remain()); 
        document.getElementById("ovladac_feeder").classList.toggle("on", st.feeder != 0);
        document.getElementById("ovladac_fan").classList.toggle("on", st.fan != 0);
        document.getElementById("ovladac_pump").classList.toggle("on", st.pump != 0);
        document.getElementById("rssi").textContent = st["rssi"];
        document.getElementById("netstatus").classList.remove("neterror");
        document.getElementById("mototempmax").hidden = st["motor_temp_ok"] == 1;
        document.getElementById("manualcontrolpanel").hidden = st.mode != 1;
        document.getElementById("man_feeder").classList.toggle("active",st.feeder != 0);
        document.getElementById("man_fan").classList.toggle("active",st.fan != 0);        
        document.getElementById("zasobnik").classList.toggle("open", st.tray_open != 0);
        document.getElementById("simul_temp").hidden = st["temp_sim"] == 0;
    };
    Controller.on_error = function(x,y) {
        document.getElementById("netstatus").classList.add("neterror");
        console.error(x,y);
    };
    document.getElementById("prepnout_rezim").addEventListener("click",async function() {
        const new_mode = 1-parseInt(Controller.config.m);
        ignore_man_change = true;
        this.disabled = true;
        await Controller.set_config({"m":new_mode});
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
        dialog_nastaveni_teploty("tout","tsoutaddr","touts");
    });
    el = document.getElementById("vstupni_teplota").parentNode;
    el.addEventListener("click",function(){
        dialog_nastaveni_teploty("tin","tsinaddr","tins");        
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
        ignore_man_change = true;
        Controller.man.feeder = Controller.status.feeder == 0;
        this.classList.toggle("active");
        
    });
    el = document.getElementById("man_fan");
    el.addEventListener("click", function() {
        ignore_man_change = true;
        Controller.man.fan = Controller.status.fan == 0;
        this.classList.toggle("active");
    });
    el = document.getElementById("man_fan_speed");    
    el.addEventListener("change",function(){
        ignore_man_change = true;
        Controller.man.fan_speed = this.value; 
    });
    el.value = 100;
    el = document.getElementById("pump_active_forever");
    el.addEventListener("change",function(){
        Controller.man.force_pump = this.checked;
        
    })

    connection.onconnect = async function() {
        let data = await connection.send_request(6,{});        
        document.getElementById("ssid").textContent = parseTextSector(data);
        
        
    }
    Controller.update_status_cycle();
    Controller.update_stats_cycle();
    await Controller.read_config();

}

 
)###");
constexpr auto embedded_style_css = string_constant(R"###(
html {
    background-color: #2f2a2a;
    color: white;
    font-family: sans-serif;

}

.varovani {
    text-align: center;
    padding: 1em;
    background-color: yellow;
    color: red;
    font-weight: bold;
    font-size: 1.2em;
}


#manualcontrolpanel {
    position: sticky;
    border: 1px solid;
    bottom: 5px;
    background-color: #000000;
    margin: 0 auto;
    width: 90vw;
    max-width: 500px;
    text-align: center;
}

#manualcontrolpanel .buttons {
    display: flex;
    justify-content: space-around;
    height: 5em;
    align-items: stretch;
}
#manualcontrolpanel .buttons > div {width: 40%;}

div#rssi::after {
    content: "dbm";
}

.wifi {
    width: 32px;
    height: 30px;
    border: 2px solid transparent;
    border-top: 2px solid currentColor;
    border-radius: 30px;
    position: relative;
    box-sizing: border-box;
}

.wifi::before {
    box-sizing: border-box;
    width: 24px;
    height: 22px;
    border: 2px solid transparent;
    border-top: 2px solid currentColor;
    border-radius: 30px;    
    position: absolute;
    content: "";
    left:2px;
    top: 3px;
}
.wifi::after {
    box-sizing: border-box;
    width: 16px;
    height: 14px;
    border: 2px solid transparent;
    border-top: 2px solid currentColor;
    border-radius: 30px;
    position: absolute;
    content: "•";
    left:6px;
    top: 8px;
    text-align: center;
    line-height: 9px;
    vertical-align:middle;
}

.chyba {
    font-size:  0.8em;
    color:  red;
    text-align: right;
    display: none;
}

.chyba.zobrazena {
    display: block;
}

.buttonpanel {
    display: flex;
    justify-content: flex-end;
    gap: 5px;
    margin-top: 1em;
}

.buttonpanel > button {
    min-width: 5em;
}

button {
    font-size: 1.1em;
    padding: 0.5em;
}

.okno.section {
    position:absolute;
    border: 1px solid gray;
    max-width: 30em;
    left:0;
    right:0;
    margin: auto;
    top: 8em;
    padding: 2em 1em 1em 1em;
    background-color:black;
    box-shadow: 5px 5px 5px 0px #00000091;
}

.zasobnik .label {
    position: absolute;
    left: 0;
    right: 0;
    top: 0;
}

.zasobnik .fill {
    background-color: #a98559;
    position: absolute;
    left: 0;
    right: 0;
    bottom: 0;
}

.zasobnik .bok1  {
    position: absolute;
    left: 0;
    bottom: 0;
    height:0;
    width:0;
    border-top: 7rem solid transparent;
    border-left: 5rem solid;
}

.zasobnik .bok1.b {
    color: #2f2a2a;
    transform: translate(-2px,2px)
}

.zasobnik .bok2  {
    position: absolute;
    right: 0;
    bottom: 0;
    height:0;
    width:0;
    border-top: 8rem solid transparent;
    border-right: 2rem solid;
}

.zasobnik .bok2.b {
    color: #2f2a2a;
    transform: translate(2px,2px)
}

div.zasobnik {
    width: 10em;
    height: 8.7em;
    border-left: 2px solid;
    border-right: 2px solid;
    border-bottom: 2px solid;
    position: relative;
}

@keyframes trayopenanim {
    0% {opacity: 100%;}
    49% {opacity: 100%;}
    50% {opacity: 0%;}
    99% {opacity: 0%;}
    100% {opacity: 100%;}
}


div.zasobnik.open .fill {
    animation: trayopenanim 1s infinite;
}
div.zasobnik.open::after {
    content: "Zásobník otevřený";
    display:block;
    position:absolute;
    top: 2em;
    left: 0.5em;
    right: 0;
    text-align: center
}

.ovladace {
    display: flex;
    /* align-items: stretch; */
    cursor: pointer;
    gap: 2px;
    flex-direction: column;
    width: 10em;
    /* height: 12em; */
    /* padding-top: 2.7em; */
    padding-bottom: 1.5em;
}

div#mototempmax {
    position: absolute;
    right: 0.7em;
    top:  0;
    bottom: 0;
    height: 1.4em;
    margin: auto;
    width: 1.4em;
    background-color: red;
    border-radius: 1em;
    border:  3px solid red;
}
div#mototempmax::before {
    content: "\1F321";
    color: yellow;
    font-size: 1.1em;
}

.ovladace > div {
    flex-grow: 1;
    align-content: center;
    border-radius: 1em;
    background-color: #444;
    color: #888;
    position: relative;
}

.ovladace > div.on {
    background-color: #477f30;
    color: yellow;
    font-weight: bold;
}

.prvekspopisem {
    text-align: center;
    position: relative;
cursor: pointer;
}

.prvekspopisem > span, .ovladace > span {
    display: block;
    padding: 1em;
}

.prvekspopisem:active {
    transform: translate(1px,1px);
}

.prvekspopisem:hover::after {
    position: absolute;
    background-color: #88888852;
    left: 0;
    top: 30px;
    right: 0px;
    bottom: 30px;
    content:"";
    display:block;
    box-shadow: 0 0 10px 14px #8885;
}

.dashboard {
    display: flex;
    gap: 5px;
    justify-content: space-around;
    flex-wrap: wrap;
}

.dashboard > div {
    width: 10em;
    text-align: center;
}
.teplomer {
    border-top: 2px solid;
    border-left: 2px solid;
    border-right: 2px solid;
    border-bottom: 2px solid #0000;
    width: 10em;
    height: 10em;
    border-radius: 10em;
    position: relative;
}

.teplomer .label {
    position:absolute;   
    left:0;
    right: 0;
}

.teplomer .label.cur_temp {
    bottom: 2rem;
    text-align: center;
    font-size: 1.5em;
    width: 2.5em;
    margin:  auto;
    border-radius: 0.5em;
    background-color: #444;
    color: yellow;
}
.teplomer .label.amp_temp::before {
    content:"trend: ";
}
.teplomer .label.amp_temp {
    bottom: 1rem;
    text-align: center;
    font-size: 0.8em;
    width: 6em;
    margin:  auto;
}
.teplomer .label.t20 {
    bottom: 1.5em;
    text-align:left;
    padding-left: 1.3em;
}
.teplomer .label.t40 {
    bottom: 6em;
    text-align:left;
    padding-left: 0.5em;
}
.teplomer .label.t60 {
    top: 0.1em;
    text-align:center;
}
.teplomer .label.t80 {
    bottom: 6em;
    text-align:right;
    padding-right: 0.5em;
}
.teplomer .label.t100 {
    bottom: 1.5em;
    text-align:right;
    padding-right: 1.3em;
}
.teplomer .needle {
    width: 9em;
    position: absolute;
    left: 0;
    right: 0;
    top: 0;
    bottom: 0;
    margin: auto;
    height: 0px;
    transition: 1s transform;
}

.teplomer .max.needle::before {
    border: 1px solid red;     
}
.teplomer .min.needle::before {
    border: 1px solid green;    
}

.teplomer .needle::before {
    border: 2px solid;
    display: block;
    content: "";
    font-size: 0px;
    width: 50%;
    margin-top: -2px;
}
.teplomer .needle::after {
    left: 0;
    right: 0;
    top: 0;
    bottom: 0;
    width: 1em;
    height: 1em;
    position:absolute;
    content: "";
    background-color: currentcolor;
    margin:auto;
    border-radius: 1em;
    
}

body {
    margin: 0;
    overflow-x:hidden;
}

.button {
    cursor: pointer;
    align-content: center;
    text-align: center;
    position: relative;
    box-sizing:content-box;
}

#manualcontrolpanel .button {
    background-color: #444;
    border-radius: 1em;
}

#manualcontrolpanel .button.active {
    background-color: #9fff26;
    border-radius: 1em;
    color: black;
}

.iconfan::after {
    content:"\274A";
    display:block;
    font-size: 1.4em;
    margin-top:  -0.2em;
    margin-bottom:  -0.2em;
}

 .iconfeeder {
    display:block;
    border: 1px solid;
    height: 0.4em;
    width: 1.4em;
    margin: 0.4em auto;
    position: relative;
    background-color: currentColor;
    border-radius: 5px;
}
.iconfeeder::before,
.iconfeeder::after {
    content: "";
    display: block;
    position: absolute;
    border: 1px solid;
    height: 1.4em;
    width: 2px;
    top: -0.55em;
    left: 0.25em;
    background-color: currentColor;
    transform: rotate(40deg)
}

.iconfeeder::after {
    left: 0.9em;
}

.ovladace  .icon {
    width: 1.5em;
    height: 1.4em;
    margin-left: 1em;
    position: absolute;
    left: 0.7em;
    top:  0;
    bottom: 0;
    margin: auto;
}

.iconpump {
    display: block;
    width: 1.4em;
    height: 1.4em;
    border:  1px solid;
    box-sizing: border-box;
    border-radius: 1em;
    position: relative;
}
.iconpump::after {
    position: absolute;
    width: 0;
    height: 0;
    left: 0;
    top: 0;
    border-top: 0.9em solid;
    border-left: 0.9em solid transparent;
    content: "";
    transform: translate(-0.1em, 0.18em) scale(1.5, 1) rotate(45deg);
}

.button:hover {
    border: 2px solid;
    margin: -2px -2px;   
}

.button:active {
    transform: translate(1px,1px);
    border: 2px groove;
}


.header {
    text-align: center;
    background-color: #393c4fd4;
    padding: 5px;
    border-bottom: 1px solid #585f8d;
    font-weight: bold;
}
.header::after {
    content: "";
    display:inline-block;
    width: 0.7em;
    height: 0.7em;
    background-color: green;
    border-radius: 1em;
    margin-left: 0.5em;
    vertical-align: baseline;
}

.header.neterror::after {
    background-color: red;
}

.header.netconnecting::after {
    animation: netconnectingAnimation 4s;
    animation-fill-mode: both;
}

@keyframes netconnectingAnimation {
    0% {
        background-color: green;
    }
    50% {
        background-color: green;
    }
    100% {
        background-color: yellow;
    }
}

#stav::before {
    display:block;
    content:"Nahrávám...";"
    font-variant-caps:small-caps;
    font-weight: bold;
    text-align: center;
    flex-grow: 1;
    font-size: 1.2em;
}

div#stav {
    display: flex;
    justify-content: space-between;
    align-items: center;
    border-bottom: 1px solid;
}

div#stav > * {
    height: 3em;
    width: 6em;
    align-content: center;
}

#stav .button {
    background-color:#878181;
    color:#ffffff;
    font-weight: bold;
}

#stav.mode1 .button {
    background-color:green;
    color:#ffffff;
}

#stav.mode2 .button {
    background-color:red;
    color:yellow;
    font-weight: bold;
}

#stav.mode0::before {
    content:"Neznámý";
}
#stav.mode1::before {
    content:"Ruční ovládání";
    color: #ff9b9b;
}


#stav .button::before {
    content: "RUČNĚ";
}

#stav.op1 .button::before {
    content: "STOP";
}

#stav.op0 .button::before {
    content: "AUTOMAT";
}

#stav.mode2{
    background-color: #445344;
}

#stav.mode2.automode0::before {
    content:"Automaticky / Plný výkon";
}
#stav.mode2.automode1::before {
    content:"Automaticky / Snížený výkon";
}
#stav.mode2.automode2::before {
    content:"Automaticky / Chlazení";
}
#stav.mode3::before {
    content:"STOP";
}
#stav.mode4::before {
    content:"Ostatní";
}

.sub_section .sectionname {
    text-align: center;
    border-bottom: 2px solid #FFF4;
}

.sub_section.unhide {
    max-height: 1.3rem;
    overflow-y: hidden;
    transition: 0.5s;
}
.sub_section.unhide.unhidden {
    max-height: 19em;
}
.sub_section.unhide > .sectionname {
    cursor: pointer;
}

.sub_section.unhide >.sectionname::after {
    content: "▾";
    padding: 0 0 0  1em;
}

.sub_section.unhide.unhidden > .sectionname::after {
    content: "▴";
}

.section >.sectionname {
    position: absolute;
    top: -0.8em;
    left: 0.5em;
    font-size: 1.2em;
    padding: 0.2em;
    background-color: #2f2a2a;
}

#nastav_teplotu .sectionname.black {
    display:none;
}

#nastav_teplotu[data-field="tin"] .vstupni {
    display: block;
}
#nastav_teplotu[data-field="tout"] .vystupni {
    display: block;
}

.section.okno >.sectionname {
    background-color:black;
    border-radius: 1em;
    padding: 0.2em 1em;
    border-top: 2px solid gray;
}

.section label {
    display: flex;
    justify-content: space-between;
    padding: 5px;
    gap: 1em;
    align-items: center;
}

.section {
    border: 1px solid;
    /* min-width: 30em; */
    margin-top: 1em;
    padding-top: 1em;
    position: relative;
}


.sect_container {
    display: flex;
}

.wifiinfo {
    position: relative;
}

.wifiinfo .wifi {
    position: absolute;
    left: 0;
    top: 1em;
    right: 0;
    bottom: 0;
    margin: auto;
    transform: scale(2.0);
    
}

.wifiinfo .label {
    text-align: center;
    font-size: 1.1em;
    margin-top:  5rem;
    border-radius: 0.5em;
    background-color: #444;
}


input {
    border: 1px solid transparent;
    color: wheat;
    font-size: 1.1em;
    background-color: #212121;
    border-bottom: 1px solid #888;
    border-radius: 5px;
}

#manualcontrolpanel .slider input {
    width: 100%;
    box-sizing:border-box;    
}
#manualcontrolpanel .slider {    
    margin-right: 10px;
    margin-left: 50%;
    margin-top: 0.5em;
}


input[type=number]{
    text-align: center;
    font-size: 1.4em;
    width: 4em;
}

input.valerror {
    background-color: #800;
    color: #FF0;
}

#nastav_wifi input {
    text-align: center;
}

.subsection > .sectionname {
        color:  #FF8;
        border-bottom: 2px solid #fff3;
})###");

