//@require forms.js
//@html control_box.html
//@style control_box.css

FormView.controls["X-CONTROLBOX"] = class extends FormViewControl {
    #fields;
    constructor(el) {
        super(el);
        const f = FormView.load("controlBox");
        f.mount(el);
        this.#fields = f.get_fields();        
    }

    get() {
        return this.#fields;
    }
    combine(a,b) {return b;}

}