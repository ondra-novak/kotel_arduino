class FormViewControl {
    #el;
    static eventMap = new WeakMap;
    constructor(e) {this.#el = e;}
    on(event, fn) {
        let s = FormViewControl.eventMap.get(this.#el);
        if (!s) {
            s = {};
            FormViewControl.eventMap.set(this.#el, s);
        }
        if (s[event]) this.#el.removeEventListener(event, s[event]);
        if (fn == null) {
            delete s[event]
        } else {
            s[event] = fn;
            this.#el.addEventListener(event, s[event]);
        }
    }
    get() {return this.#el.textContent;}
    set(v) {this.#el.textContent = v;}
    element() {return this.#el};
    hide(h) {
        this.#el.hidden = h;
    }
    is_hidden() {
        return this.#el.hidden;
    }
    disable(h) {
        this.#el.disabled = h;
    }
    is_disable() {
        return this.#el.disabled;
    }
    set_attrib(a, v) {
        this.#el.setAttribute(a, v);
    }
    get_attrib(a) {
        return this.#el.getAttribute(a);
    }
    set_classlist(a, v) {
        this.#el.classList.toggle(a,v);
    }
    get_classlist(a) {
        return this.#el.classList.contains(a);
    }
    set_style(s, v) {
        this.#el.style[s] = v;        
    }
    get_style(s) {
        return this.#el.style[s];
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
        if (!t) throw new Error(`template ${template_id} not found`);
        return FormView.fromTemplate(t);
    }

    #add_control(n,c) {
        if (!c.combine) {
            c.combine =  (a,b) => {
                if (a && b && typeof a =="object" && typeof b == "object") {
                    return Object.assign(a,b);
                } else {
                    return a || b;
                }
            }            
        }
        if (!this.#controls[n]) this.#controls[n] = [c];
        else this.#controls[n].push(c);
        
    }
    #split_kv(s) {
        return s.split(',').map(s=>s.split('=',2));
    }

    constructor(el) {
        this.#root = el;        
        const lists = el.querySelectorAll("[data-key]");
        for (const e of lists) { 
            if (e.dataset.name) {
                this.#add_control(e.dataset.name,new FormView.controls["TEMPLATE"](e));
            }
        }

        let elms = el.querySelectorAll("[data-name],[data-attr],[data-classlist],[data-hide],[data-style],[data-disable]");        
        elms = [el, ...elms];        
        for (const e of elms) {
            const type = FormView.controls[e.tagName];
            let c;
            if (!type) c = new FormViewControl(e);
            else c = new type(e);

            if (e.dataset.name) {
                this.#add_control(e.dataset.name, c);
            }
            if (e.dataset.attr) {
                this.#split_kv(e.dataset.attr).forEach(kv=>{
                    let k = kv[0];
                    let v = kv[1];
                    if (!v) v = k;
                    this.#add_control(v,{
                        set: (v)=>c.set_attrib(k,v),
                        get: ()=>c.get_attrib(k)
                    })
                });
            }
            if (e.dataset.classlist) {
                this.#split_kv(e.dataset.classlist).forEach(kv=>{
                    let k = kv[0];
                    let v = kv[1];
                    if (!v) v = k;
                    if (v.startsWith("!")) {
                        v = v.substring(1);
                        this.#add_control(v,{
                            set: (v)=>c.set_classlist(k,!v),
                            get: ()=>!c.get_classlist(k)
                    })} else {
                        this.#add_control(v,{
                            set: (v)=>c.set_classlist(k,v),
                            get: ()=>c.get_classlist(k)
                        })
                    }
                });
            }
            if (e.dataset.style) {
                this.#split_kv(e.dataset.style).forEach(kv=>{
                    let k = kv[0];
                    let v = kv[1];
                    if (!v) v = k;
                    this.#add_control(v,{
                        set: (v)=>c.set_style(k,v),
                        get: ()=>c.get_style(k)
                    })                    
                });
            }
            if (e.dataset.hide) {
                const v = e.dataset.hide;
                if (v.startsWith("!")) {
                    this.#add_control(v.substring(1), {
                        set:(v)=>c.hide(!v),
                        get:()=>!c.is_hidden()
                    });
                } else {
                    this.#add_control(v, {
                        set:(v)=>c.hide(v),
                        get:()=>c.is_hidden()
                    });
                }
            }
            if (e.dataset.disable) {
                const v = e.datset.disable
                if (v.startsWith("!")) {
                    this.#add_control(v.substring(1), {
                        set:(v)=>c.disable(!v),
                        get:()=>!c.is_disable()
                    });
                } else {
                    this.#add_control(v, {
                        set:(v)=>c.disable(v),
                        get:()=>c.is_disable()
                    });
                }
            }
        }
    }


    mount(parent = document.body, before = null) {
        parent.insertBefore(this.#root, before);
    }
    

    unmount() {
        if (this.#root.parentNode) this.#root.parentNode.removeChild(this.#root);
    }
    on(field,event,fn) {
        this.get_controls(field).forEach(x=>x.on(event, fn));
    }

    get_fields() {
        return new Proxy(
            this.#controls,
            {
                get(target, prop) {
                    if (target[prop]) {
                        return target[prop].reduce((prev, cur)=>cur.combine(prev,cur.get()), null);
                    } else {
                        return undefined;
                    }
                },
                set(target, prop, value) {
                    const c = target[prop];
                    if (c) {
                        if (value instanceof Promise) {
                            value.then(r=>c.forEach(x=>x.set(r)));
                        } else  {
                            c.forEach(x=>x.set(value));
                        }
                        return true;
                    }
                    return false;
                },
                has(target, prop) {
                    return !!target[prop];
                },
                ownKeys(target) {
                    return Object.keys(target);
                },
                getOwnPropertyDescriptor(target, prop) {
                    const c = target[prop];
                    if (c) {
                        return {
                            enumerable: true,
                            configurable: true
                        };
                    }
                    return undefined;
                }
            }
        );
    }


    get_controls(field) {
        return this.#controls[field] || [];
    }
    get(fields) {        
        if (Array.isArray(fields)) {
            return Object.fromEntries(fields.map(f=>[f,this.get(f)]));
        } else {
            return this.get_controls(fields).reduce((prev,cur)=>cur.combine(prev, cur.get()),null);
        }
    }
    get_all() {
        const out = {};
        for (const [n,el] of Object.entries(this.#controls)) {
            out[n] = el.reduce((p,c)=>c.combine(p,c.get()),null);
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
                    els.forEach(e=>e.set(resolved));        
                });
            } else {
                els.forEach(e=>e.set(v));
            }
        }
    }
}

FormView.controls = {
    INPUT:class extends FormViewControl{
        constructor(el) {

            function validate() {
                    // přečteme hodnotu jako číslo
                    let value = el.valueAsNumber;
                    if (isNaN(value)) {
                        value = 0;
                    }
                    const min = parseFloat(el.min) || -Infinity; // pokud není min, žádný limit
                    const max = parseFloat(el.max) || Infinity;  // pokud není max, žádný limit
                    if (value < min) value = min;
                    if (value > max) value = max;
                    el.value = value;                
            }

            super(el);
            if (el.getAttribute("type") == "number") {
                setTimeout(()=>validate(el),0);
                el.addEventListener("change",()=>{
                    validate(el);
                });
            }
        }
        
        get() {
            const e = this.element();
            if (e.type === "checkbox") {
                if (e.dataset.mask) {
                    return e.checked?parseInt(e.dataset.mask):0;                }
                if (e.value) return Object.fromEntries([e.value, e.checked]);
                return e.checked;
            } else if (e.type === "radio") {
                if (e.value) return e.checked?e.value:null;
                return e.checked;        
            } else if (e.type === "number") {
                return e.valueAsNumber;
            } else if (e.type === "date") {
                return e.valueAsDate;
            } else {
                return e.value;
            }
        }
        set(v) {
            const e = this.element();
            if (e.type === "checkbox") {
                if (e.dataset.mask) {
                    e.checked = !!(v & parseInt(e.dataset.mask));
                } else  if (e.value) e.checked = v[e.value];                    
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
        combine(a,b) {
            if (this.element().dataset.mask && typeof a == "number" && typeof b == "number")  {
                return a | b;
            } else {
                return a || b;
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
    BUTTON: class extends FormViewControl {
        on(event, fn) {
            if (event != "click") return super.on(event, fn);
            else {
                super.on(event,async (ev)=>{
                    const st = this.is_disable();
                    this.disable(true);
                    try {
                        await Promise.resolve(fn(ev));
                        this.disable(st);                        
                    } catch (e) {
                        this.disable(st);
                        throw e;
                    }
                });
            }
        }    
    },
    TEMPLATE: class extends FormViewControl {            
        #list = new Map;
        #last = null;
        #parent = null;
        #events = {};

        constructor(el) {
            super(el);
            this.#last = document.createComment("TEMPLATE");                
            this.#parent = el.parentNode;
            this.#parent.insertBefore(this.#last, el);
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
            if (model) {
                const allflds = this.#list.map(x=>x.get_controls(model)).flat();
                return allflds.reduce((p,c)=>c.combine(p,c.get()), null);                    
            }
            let out = [];
            this.#list.forEach((v,k)=>{
                const r = v.get_all();
                r[kf] = k;
                out.push(r);
            });
            return {data: out};
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
                        const ev = this.#events.mount;
                        queueMicrotask(()=>ev(f));
                    }
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
                        const ev = this.#events.unmount;
                        queueMicrotask(()=>ev(f));
                    }
                    f.unmount();
                    this.#list.delete(k);
                }
            }
        }
        on(event, fn) {
            this.#events[event] = fn;
        }
    }
}

class ModalDialog extends FormView {
    #promok = null;
    static fromTemplate(t_element) {
        var cloned = document.importNode(t_element.content, true);        
        let dlg = document.createElement("DIALOG");
        dlg.appendChild(cloned);
        
        for (const attr of t_element.attributes || []) {
            if (attr.name !== "id" && attr.name !== "name") {
                dlg.setAttribute(attr.name, attr.value);
            }
        }        
        return new ModalDialog(dlg);        
    }
    constructor(el)  {
        super(el);
        el.addEventListener("close", ()=>{
            this.#finalize(null);
            super.unmount();
        });
    }
    
    static load(template_id) {
        var t = document.getElementById(template_id);
        if (!t) throw new Error("template not found");
        return ModalDialog.fromTemplate(t);
    }

    #finalize(result) {        
        const p = this.#promok;
        this.#promok = null;
        if (p) {
            p(result);
        }
    }

    close(result) {
        this.#finalize(result);
        this.get_root().close();                
    }

    async do_modal(close_butt = null) {
        if (close_butt) {
            super.on(close_butt,"click",()=>this.get_root().close());
        }
        return new Promise((ok)=>{
            this.#promok = ok;
            this.mount();
        })
    }


    mount(close_cb, cancel_button) {
        super.mount();
        this.get_root().showModal();                
    }
    unmount() {
        this.get_root().close();
    }
}