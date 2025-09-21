//@html adv_config.html
//@require forms.js
//@require websocketclient.js
//@require protocol.js

function configureAdvanced(cfgobj,ws) {
    const f = ModalDialog.load("advConfig");
    const cfg = cfgobj.get();
    const newcfg = {};
    const flds = f.get_fields();    
    const spd = parseInt(cfg["fd.speed"]);
    flds.speed = spd;
    flds.fannc = cfg.fannc;
    flds.dspli = cfg.dspli
    const save = async ()=>{
        newcfg.fannc = flds.fannc;
        newcfg.dspli = flds.dspli;
        if (spd != flds.speed) {
            newcfg["fd.speed"] = flds.speed;
        }
        await Promise.resolve(cfgobj.set(newcfg));
        f.close(1);
    };
    f.on("rst","click", ()=>{
        save();
        ws.send_request(WsReqCmd.reset);
    })
    f.on("bok","click",save);
    f.on("badv","click", ()=>flds.adv = !flds.adv);
    return f.do_modal("bst");


}