//@require forms.js
//@html net_config.html
//@style net_config.css

FormView.controls["X-IPINPUT"] = class extends FormViewControl {
    #inputs = [];
    constructor(el) {
        super(el);
        for (let i = 0; i < 4; ++i) {
            const e = document.createElement("INPUT");
            [["type","text"],["maxlength","3"]].forEach(x=>e.setAttribute(x[0],x[1]));
            if (i < 3) {
                e.addEventListener("keydown",(ev)=>{
                    if (ev.key == " " || ev.key == "Enter") {
                        e.nextElementSibling.focus();
                        ev.stopPropagation();
                        ev.preventDefault();
                    }
                });
                e.addEventListener("input",()=>{
                const v = parseInt(e.value);
                if (isNaN(v)) return;
                    if (v > 25 || e.value.length == 3) e.nextElementSibling.focus();
                });

            } 
            e.addEventListener("change",()=>{
                const v = parseInt(e.value);
                if (isNaN(v) || v < 0 || v > 255) {
                    e.value = "0";
                    queueMicrotask(()=>e.focus());
                }
            })
            e.addEventListener("focus", () => {
                e.select();
            });
            el.appendChild(e);
            this.#inputs.push(e);
            e.style.flexGrow = "1";
            e.style.display="block";
            e.style.textAlign="center";
            e.style.width="1px"
        }
        el.style.display="inline-flex";     
        
    }

    set(v) {
        v.split(".").forEach((v,idx)=>this.#inputs[idx].value = v);
    }
    get() {
        return this.#inputs.map(x=>`${parseInt(x.value)}`).join(".");
    }
    
}

/*
net.ip=0.0.0.0
net.dns=0.0.0.0
net.gateway=0.0.0.0
net.netmask=255.255.255.0
*/
function configureNetworkAdv(cfgobj) {
    const f = ModalDialog.load("netSettings");
    const cfg = cfgobj.get();
    const newcfg = {};
    const flds = f.get_fields();    
    flds.ipaddr = cfg["net.ip"];
    flds.dns = cfg["net.dns"];
    flds.gateway = cfg["net.gateway"];
    flds.mask = cfg["net.netmask"];

    f.on("bok","click",async ()=>{
        newcfg["net.ip"] = flds.ipaddr;
        newcfg["net.dns"] = flds.dns;
        newcfg["net.gateway"] = flds.gateway;
        newcfg["net.netmask"] = flds.mask;
        await Promise.resolve(cfgobj.set(newcfg));
        f.close(1);
    });
    f.on("badv","click", ()=>flds.adv = !flds.adv);
    return f.do_modal("bst");


}

function configureNetwork(cfgobj) {
    const f = ModalDialog.load("netConfig");
    const cfg = cfgobj.get();
    const newcfg = {};
    const flds = f.get_fields();    
    const encr = ["WEP","WPA","WPA2","WPA2 E","WPA3","none","AUTO"];
    flds.ap = cfg["wifi.ssid"];
    flds.pwd = cfg["wifi.password"];
    flds.found = false;
    f.on("scan","click", async()=>{
        await fetch("/api/scan_wifi",{method:"POST"});
        while(true) {
            try {
                await new Promise(ok=>setTimeout(ok, 1000));
                const r = await ( await fetch("/api/scan_wifi")).text();
                const tbl = r.split("\n").map(x=>x.split(','))
                    .map((x,idx)=>({
                        idx: idx,
                        ssid: x[2],
                        encr: encr[x[1]],
                        sign: x[0]
                    }));
                flds.list = tbl;
                flds.found = true;
                return;

            } catch (e) {
                console.log("no result yet", e);
            }
        }
    });
    f.on("list","mount", (f)=>{
        f.on("list","click",()=>{
            flds.ap = f.get("ssid");
        });
    });
    f.on("adv","click", (f)=>{
        configureNetworkAdv(cfgobj);
    })
    f.on("bok","click",async ()=>{
        newcfg["wifi.ssid"] = flds.ap;
        if (cfg["wifi.password"] != flds.pwd) {
            newcfg["wifi.password"] = flds.pwd;            
        }
        await Promise.resolve(cfgobj.set(newcfg));
        f.close(1);
    });
    f.on("badv","click", ()=>flds.adv = !flds.adv);
    return f.do_modal("bst");
}