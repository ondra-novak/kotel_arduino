//@require forms.js
//@html temp_gauge.html
//@style temp_gauge.css

FormView.controls["X-TEMPGAUGE"] = class extends FormViewControl {

    #fields;

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

    get() {
        const self = this;
        return new Proxy({
            cur:0,min:0,max:0,amp:0
        },{
            get: (t,p) => {return t[p];},
            set: (t,p,v) => {
                if (p == "cur") {
                    self.#fields.cur_tmp = self.#gstyle(v);
                    self.#fields.cur_tmp_value = self.#gtext(v);
                    t[p] =v;
                    return true;
                } else if (p =="min") {
                    t[p] = v;
                    self.#fields.min_tmp = self.#gstyle(v);
                    return true;
                } else if (p =="max") {
                    t[p] = v;
                    self.#fields.max_tmp = self.#gstyle(v);
                    return true;
                } else if (p =="amp") {
                    t[p] = v;
                    self.#fields.amp_tmp_value = self.#gtext(v);
                    return true;
                } else{
                    return false;
                }
            },
            has: (target, prop) => {
                return !!target[prop];
            },
            ownKeys: (target) => {
                return Object.keys(target);
            },
            getOwnPropertyDescriptor: (target, prop) => {
                const c = target[prop];
                if (c) {
                    return {
                        enumerable: true,
                        configurable: true
                    };
                }
                return undefined;
            }       
        })
    }
    
    set(v) {
        this.get().cur = v;
    }

}