//@require forms.js
//@html netstat.html
//@style netstat.css

FormView.controls["X-NETSTAT"] = class extends FormViewControl {
    
    #fields
    
    constructor(el) {
        super(el);
        const f = FormView.load("netStat");
        f.mount(el);
        this.#fields = f.get_fields();
    }
    
    get() {
        return this.#fields;
    }
    
}
    