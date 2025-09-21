//@html tray_config.html
//@requires forms.js
//@requires protocol.js

// If you want VSCode to improve IntelliSense, you can use JSDoc imports:
/// <reference path="./forms.js" />
/// <reference path="./protocol.js" />

async function configureTray(cfgobj) {
    const f = ModalDialog.load("trayConfig");
    const cfg = cfgobj.get();
    const newcfg = {};
    const flds = f.get_fields();
    flds.traykg = cfg.traykg;
    flds.bgkg = cfg.bgkg;
    flds.mjkg = cfg.hval
    f.on("bok","click",async ()=>{
        newcfg.traykg = flds.traykg;
        newcfg.bgkg = flds.bgkg;
        newcfg.hval = flds.mjkg;
        await Promise.resolve(cfgobj.set(newcfg));
        f.close(f.get(["fuel","unit"]));
    })
    return f.do_modal("bst");
}

