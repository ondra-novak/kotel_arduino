//@html menu_settings.html
//@style menu_settings.css
//@require forms.js

class MenuSettings {


    f = FormView.load("menuSettings");
    
    #close_fn;
    #selected;
    #verstr;

    constructor() {
        this.#verstr = "";
        const el = this.f.get_root();
        el.addEventListener("click",(ev)=>{            
            ev.stopPropagation();
            const r = Array.prototype.findIndex.call(el.children, x=>x === ev.target);
            if (r) this.close(r);
        });
    }

    set_ver(ver) {
        this.#verstr = ver;
    }

    open() {
        return new Promise(ok=>{
            this.#selected = ok;
            if (!this.#close_fn) {
                this.#close_fn = ()=>{
                    this.close(-1);
                }        
                setTimeout(()=>document.body.addEventListener("click", this.#close_fn),0);
                this.f.set({"version": this.#verstr});
                this.f.mount();
            }
        });        
    }
    close(v) {
        if (this.#close_fn) {
            document.body.removeEventListener("click", this.#close_fn);
            this.#close_fn = null;
            this.f.unmount();
            this.#selected(v);
        }
    }

};
