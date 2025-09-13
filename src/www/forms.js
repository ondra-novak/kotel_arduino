


class FormViewControl {
    #el;
    #events = {};
    constructor(e) {this.#el = e;}
    on(event, fn) {
        if (this.#events[event]) this.#el.removeEventListener(event, this.#events[event]);
        this.#events[event] = fn;
        this.#el.addEventListener(event, this.#events[event]);
    }
    get() {return null;}
    set(v) {this.#el.textContent = v;}
    element() {return this.#el};
    hide(h) {
        this.#el.hidden = h;
    }
};

class FormView {
    #root;
    #controls = {};

    static listMap = new WeakMap;

    static fromTemplate(t_element) {
        var cloned = document.importNode(t_element.content, true);
        let children = Array.from(cloned.childNodes).filter(n => n.nodeType === 1);
        let root;
        if (children.length === 1) {
            root = children[0];
        } else {
            root = document.createElement("div");
            root.append(...children);
        }
        return new FormView(root);        
    }
    
    static load(template_id) {
        var t = document.getElementById(template_id);
        if (!t) throw new Error("template not found");
        return FormView.fromTemplate(t);
    }

    static AttribControl = class extends FormViewControl {
        #attrib;
        constructor(el, attrib) {
            super(el);
            this.#attrib = attrib;
        }
        set(v) {
            this.element().setAttribute(this.#attrib, v);
        }
        get() {
            return this.element().getAttribute(this.#attrib);
        }
    };


    constructor(el) {
        this.#root = el;
        const lists = el.querySelectorAll("[data-key]");
        for (const e of lists) { 
            if (e.dataset.name) {
                const n = e.dataset.name;
                if (!this.#controls[n])  this.#controls[n] = [];    
                this.#controls[n].push(new FormView.controls["TEMPLATE"](e));
            }
        }
        let named = el.querySelectorAll("[data-name]");
        if (el.dataset.name) {
            named = [el, ...named];
        }
        for (const e of named) {
            let c;
            const n = e.dataset.name;
            const type = FormView.controls[e.tagName];
            if (!type) c = new FormViewControl(e);
            else c = new type(e);
            if (!this.#controls[n])  this.#controls[n] = [];
            this.#controls[n].push(c);
        }
        let attrmap = el.querySelectorAll("[data-attr]");
        if (el.dataset.attr) {
            attrmap = [el, ...attrmap];
        }
        for (const e of attrmap) {
            let c;
            const n = e.dataset.attr;
            n.split(",").forEach(x=>{
                const [attr, name] = x.split('=',2);
                if (!this.#controls[name])  this.#controls[name] = [];    
                this.#controls[name].push(new FormView.AttribControl(e, attr));
            });
        }

    }


    mount(parent = document.body, after = null) {
        if (after) {
            const n = after.nextElementSibling;
            parent.insertBefore(this.#root, n);
        } else {
            parent.appendChild(this.#root);
        }
    }
    

    unmount() {
        if (this.#root.parentNode) this.#root.parentNode.removeChild(this.#root);
    }

    get_controls(field) {
        return this.#controls[field] || [];
    }
    get(fields) {        
        if (Array.isArray(fields)) {
            return Object.fromEntries(fields.map(n=>[n,
                FormView.combine(this.get_controls(n)
                    .map(e=>e.get()))]));     
        } else {
            return FormView.combine(this.get_controls(fields)
                    .map(e=>e.get()));     
        }
    }
    get_all() {
        const out = {};
        for (const [n,el] of Object.entries(this.#controls)) {
            out[n] = FormView.combine(el.map(e=>e.get()));
        }
        return out;
    }
    get_root() {return this.#root};

    hide(field, hide = true) {
        this.get_elements(field).forEach(e=>{
            e.style.display = hide?"none":"";
        });
    }
    set(fvobj) {
        for (const [n,v] of Object.entries(fvobj)) {
            const els = this.get_controls(n);
            if (v instanceof Promise) {
                v.then(resolved => {
                    els.forEach(e=>e.set(resolved, fvobj));        
                });
            } else {
                els.forEach(e=>e.set(v, fvobj));
            }
        }
    }

    static combine(values) {
        if (Array.isArray(values)) {
            const first = values[0];
            if (typeof first == "object" && first !== null && !Array.isArray(first)) {
                return Object.assign({}, ...values);
            } else {
                for (const v of values) {
                    if (v !== null && v !== undefined) return v;
                }
                return null;
            }
        } 
    }


}

FormView.controls = {
        INPUT:class extends FormViewControl{
            get() {
                const e = this.element();
                if (e.type === "checkbox") {
                    if (e.value) return Object.fromEntries([e.value, e.checked]);
                    return e.checked;
                } else if (e.type === "radio") {
                    if (e.value) return e.checked?e.value:null;
                    return e.checked;        
                } else if (e.type === "number") {
                    return e.valueAsNumber;
                } else if (e.type === "date") {
                    return e.valueAsDate;
                }
            }
            set(v, obj) {
                const e = this.element();
                if (e.type === "checkbox") {
                    if (e.value) e.checked = v[e.value];
                    else e.checked = v;
                } else if (e.type === "radio" ) {
                    if (e.dataset.value && obj[e.dataset.value]) {
                        e.value = obj[e.dataset.value];
                    }
                    if (e.value) e.checked = e.value == v;
                    else e.checked = v;
                } else {
                    e.value = v;
                } 
            }               
        },
        TEXTAREA: class extends FormViewControl {
            get() {return this.element().value;}
            set(v) {this.element().value = v;}
        },
        SELECT: class extends FormViewControl {
            get() {return this.element().value;}
            set(v) {this.element().value = v;}
        },
        OPTION: class extends FormViewControl {
            get() {return null;}
            set(v) {
                const e = this.element();
                e.textContent = v;
            }
        },
        TEMPLATE: class extends FormViewControl {            
            #list = new Map;
            #last = null;
            #parent = null;
            #events = {};

            constructor(el) {
                super(el);
                this.#last = el;
                this.#parent = el.parentNode;
                this.#parent.removeChild(el);
            }

            key_field() {
                return this.element().dataset.key;
            }

            model_field() {
                return this.element().dataset.model;
            }
            
            get() {
                const kf = this.key_field();
                const model = this.model_field();
                let out = [];
                this.#list.forEach((v,k)=>{
                    const r = v.get_all();
                    r[kf] = k;
                    out.push(r);
                });
                if (model) return FormView.combine(out.map(x=>x[model]));
                else return {data: out};
            }

            set(v) {
                const model = this.model_field();
                if (Array.isArray(v)) {
                    this.#replace(v);
                } else if (!model || (typeof v == "object" && v !== null)) {
                    const r = !!v.replace;
                    const rm = v.remove;
                    if (r) this.#replace(v.data) ;
                    else if (v.data) this.#inc(v.data);
                    if (Array.isArray(rm)) {
                        this.#remove(rm)
                    }
                } else if (model) {
                    const o = {[model]:v};
                    this.#list.forEach((v,k)=>v.set(o))
                }
            }
            #replace(data) {
                this.#inc(data);
                const changed = new Set;
                const kf = this.key_field();
                for (const v of data) {
                    const k = v[kf];
                    changed.add(k);
                }
                const to_remove = [];
                this.#list.keys().forEach(k=>{
                    if (!changed.has(k)) to_remove.push(k);
                });
                this.#remove(to_remove);
            }
            #inc(data) {
                const kf = this.key_field();
                for (const v of data) {
                    const k = v[kf];
                    let f = this.#list.get(k);
                    if (!f) {
                        if (this.element().tagName != "TEMPLATE") {
                            f = new FormView(this.element().cloneNode(true));
                        } else {
                            f = FormView.fromTemplate(this.element());
                        }
                        f.mount(this.#parent,this.#last);
                        if (this.#events.mount) {
                            this.#events.mount(f);
                        }
                        this.#last = f.get_root();                        
                        this.#list.set(k, f);
                    }
                    f.set(v);
                }
            }
            #remove(lst) {
                for (const k of lst) {
                    const f = this.#list.get(k);
                    if (f) {
                        if (this.#events.unmount) {
                            this.#events.unmount(f);
                        }
                        f.unmount();
                        this.#list.delete(k);
                        if (f.get_root() == this.#last) {
                            this.#last = f.get_root().previousElementSibling;
                        }
                    }
                }
            }
            on(event, fn) {
                this.#events[event] = fn;
            }
        }
       
    }
