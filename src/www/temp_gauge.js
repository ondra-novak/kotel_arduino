//@require forms.js
//@require reactive.js
//@html temp_gauge.html
//@style temp_gauge.css

FormView.controls["X-TEMPGAUGE"] = class extends FormViewControl {

    #fields;
    #values = reactive({
                cur:null,min:null,max:null,amp:null
            },
            (v)=>{
                        this.#fields.cur_tmp = this.#gstyle(v.cur);
                        this.#fields.cur_tmp_value = this.#gtext(v.cur);
                        this.#fields.min_tmp = this.#gstyle(v.min);
                        this.#fields.max_tmp = this.#gstyle(v.max);            
                        this.#fields.amp_tmp_value = this.#gtext(v.amp);
                    });

    constructor(el) {
        super(el);
        const f = FormView.load("tempGauge");
        f.mount(el);
        this.#fields = f.get_fields();
    }
    
    #angle(temp) {
        let a = (temp - 20) * 270 / 80;
        if (a < 0) a = 0;
        if (a > 270) a = 270;
        return (a - 45) + "deg";
    }

    #gstyle(val) {
        if (typeof val == "number") {
            return "transform: rotate(" + this.#angle(val) + ");"
        } else {
            return "display:none";
        }
    }

    #gtext(val) {
        if (typeof val == "number" && isFinite(val))  {
            return val.toFixed(1);
        } else {
            return "--.-";
        }
    }

    get() {return this.#values}
                    
}