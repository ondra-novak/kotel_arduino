//@html tray_config.html
//@requires forms.js
//@requires protocol.js

// If you want VSCode to improve IntelliSense, you can use JSDoc imports:
/// <reference path="./forms.js" />
/// <reference path="./protocol.js" />

async function configureTray(cfgobj) {
    const f = ModalDialog.load("trayConfig");
    const cfg = cfgobj.get();
    const flds = f.get_fields();
    flds.traykg = cfg.traykg;
    flds.bgkg = cfg.bgkg;
    f.on("bok","click",async ()=>{
        cfg.traykg = flds.traykg;
        cfg.bgkg = flds.bgkg;
        await Promise.resolve(cfgobj.set(cfg));
        f.close(f.get(["addkg","addbag"]));
    })
    return f.do_modal("bst");
}

