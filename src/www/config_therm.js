//@html config_therm.html
//@requires forms.js
//@requires protocol.js

// If you want VSCode to improve IntelliSense, you can use JSDoc imports:
/// <reference path="./forms.js" />
/// <reference path="./protocol.js" />

async function configureThermometer(target, cur_cfg) {
    const f = ModalDialog.load("configTherm");
    const flds = f.get_fields();
    fetch("/api/scan_temp", {method:"POST"})
        .then(res=>res.text())
        .then(resp=>{
            flds.option =  Object.entries(parseResponse(resp))
            .map(kv=>{
                return {
                    addr: kv[0],
                    option: `${kv[1]}°C`
                };
            })
            flds.hwaddr = cur_cfg.hwaddr;
        });    
    flds.kalib=cur_cfg.kalib;
    f.on("bok","click",()=>{
        f.close(f.get(["hwaddr","kalib"]));
    });
    flds.target = target;
    return f.do_modal("bst");
}

async function configureOutputTherm(cfgobj) {
    const f = ModalDialog.load("configOutputTherm");
    const cfg = cfgobj.get();
    const newcfg = {};
    const flds = f.get_fields();
    flds.tout = cfg.tout;
    flds.tlsf = cfg.tlsf;
    flds.touts = (parseInt(cfg.touts) / 6).toFixed(1);
    f.on("ch","click",async ()=>{
        const r =await configureThermometer("Výstupní", {
            hwaddr: cfg.tsoutaddr,
            kalib: cfg.tsoutcalib60,                
        })
        if (r) {
            newcfg.tsoutaddr = r.hwaddr;
            newcfg.tsoutcalib60 = r.kalib;
        }
    });
    f.on("bok","click",async ()=>{
        newcfg.tout = flds.tout;
        newcfg.tlsf = flds.tlsf;
        if (!isNaN(flds.touts)) newcfg.touts = (flds.touts * 6).toFixed(0);
        await Promise.resolve(cfgobj.set(newcfg));
        f.close(true);
    })
    return f.do_modal("bst");

}

async function configureInputTherm(cfgobj) {
    const f = ModalDialog.load("configInputTherm");
    const cfg = cfgobj.get();
    const newcfg = {};
    const flds = f.get_fields();
    flds.tlow = cfg.tlow;
    flds.tfull = cfg.tfull;
    flds.tins = (parseInt(cfg.tins) / 6).toFixed(1);
    f.on("ch","click",async ()=>{
        const r =await configureThermometer("Vstupní", {
            hwaddr: cfg.tsinaddr,
            kalib: cfg.tsincalib60,                
        })
        if (r) {
            newcfg.tsinaddr = r.hwaddr;
            newcfg.tsincalib60 = r.kalib;
        }
    });
    f.on("bok","click",async ()=>{
        newcfg.tlow = flds.tlow;
        newcfg.tfull = flds.tfull;
        if (!isNaN(flds.trend)) newcfg.tins = (flds.tins* 6).toFixed(0);
        await Promise.resolve(cfgobj.set(newcfg));
        f.close(true);
    })
    return f.do_modal("bst");

}