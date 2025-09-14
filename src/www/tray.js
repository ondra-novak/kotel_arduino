//@require forms.js
//@html tray.html
//@style tray.css

FormView.controls["X-TRAYGAUGE"] = class extends FormViewControl {
    
    #fields;
    constructor(el) {
        super(el);
        const f = FormView.load("trayGauge");
        f.mount(el);
        this.#fields = f.get_fields();
    }
    set(v) {
        if (typeof v == "boolean") {
            this.#fields.open = v;
        } else {
            this.#fields.p = v;
            const p = v*0.01;
            const koef = 0.7;
            const f = Math.floor((Math.sqrt(koef * p) + p * (1 - Math.sqrt(koef))) * 100);
            this.#fields.h =  `height: ${f}%`;
        }
    }
    get() {
        return this.#fields;
    }

}
