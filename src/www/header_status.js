//@html header_status.html
//@style header_status.css
//@require forms.js
//@require reactive.js
/// <reference path="./forms.js" />
/// <reference path="./reactive.js" />

FormView.controls["X-HEADERSTATUS"] = class extends FormViewControl {
    #fields;
    #f;
    #ev = {};

    constructor(el) {
        super(el);
        this.#f = FormView.load("headerStatus");
        this.#fields = this.#f.get_fields();      
        this.#f.on("prepnout_rezim","click",(ev)=>{
            const fn = this.#ev["click"];
            if (fn) fn(ev);
        });
        this.#f.mount(el);
    }

    #pub_fields = reactive({
        devicetime: new Date(),
        mode:0,
        automode:0,
        op:0,
        netok:false,
        simul:false,
        op_pend: false
    },(v)=>{
        this.#fields.devicedate = v.devicetime.toLocaleDateString(undefined, { day: 'numeric', month: 'numeric' });
        this.#fields.devicetime = v.devicetime.toLocaleTimeString();
        this.#fields.stav = `stav op${v.op} mode${v.mode} automode${v.automode} ${v.op_pend?"pend":""}`;
        this.#fields.simul = v.simul;
        this.#fields.netok = v.netok;
    });

    on(ev, fn ){
        this.#ev[ev] = fn;
    }

    get() {
        return this.#pub_fields;
    }
       
    
};