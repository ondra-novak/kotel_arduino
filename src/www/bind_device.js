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
                const r = await fetch("/api/code", {method:"POST", body: fld.code});
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