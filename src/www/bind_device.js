//@require forms.js
//@require protocol.js
//@html bind_device.html
//@style bind_device.css

async function bindDeviceDialog() {
    const dlg = ModalDialog.load("bindDevice");
    const fld = dlg.get_fields();
    dlg.on("show","click",async ()=>{
        try {
            fld.show_error =false;
            const r = await fetch("/api/code", {method:"POST"});
            fld.show_error = r.status != 202;
        } catch (e) {
            fld.show_error =true;
        }
    }); 
    dlg.on("bind","click", async()=>{
        try {
            fld.code_error = false;
            fld.send_error = false;
            if (fld.code.length == 4) {
                const code = fld.code.toUpperCase();
                const r = await fetch("/api/code", {method:"POST", body: code});
                if (r.status == 200) {
                    const txt = parseResponse(await r.text());
                    dlg.close(txt);                
                }
            }
            fld.code_error = true;
        } catch (e) {
            fld.send_error = true;
        }
    })
    let res;
    do {    
        res = await dlg.do_modal();        
    } while (!res);
    return res;
}
async function bindConfig(ws) {
    const dlg = ModalDialog.load("bindConfig");
    const fld = dlg.get_fields();
    dlg.on("gen","click",async ()=>{
        fld.code = await ws.send_request(WsReqCmd.generate_code);
    });
    dlg.on("bok","click", async()=>{

        switch (parseInt(fld.op)) {
            case 0: break;
            case 1: {
                ws.set_token("");
                location.reload();
            } break;
            case 2: {        
                const tkn_text = await ws.send_request(WsReqCmd.unpair_all);
                token = parseResponse(tkn_text)["token"];
                ws.set_token(token);
            }break;
        }
        dlg.close();
    });
    

    return dlg.do_modal("bst");
}