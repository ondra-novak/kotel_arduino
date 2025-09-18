function reactive(target, callback) {
    let old_values={};
    let scheduled =false;
  
    return new Proxy(target, {
        set(obj, prop, value) {
            if (prop in obj) {
                const oldValue = obj[prop];
                if (oldValue !== value) {
                    if (!(prop in old_values)) {
                        old_values[prop] = oldValue;
                        target[prop] = value;
                        if (!scheduled) {
                            scheduled = true;
                            queueMicrotask(()=>{
                                scheduled = false;
                                let tmp = old_values;
                                old_values = {};
                                callback(target, old_values);
                            })                    
                        }
                    }
                }
                return true;
            } else {
                return false;
            }
        }
    });
}