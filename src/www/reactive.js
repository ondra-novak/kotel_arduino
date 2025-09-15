function reactive(target, callback) {
  return new Proxy(target, {
    set(obj, prop, value) {
      const oldValue = obj[prop];
      if (oldValue !== value) {
        obj[prop] = value;
        const prop_cb = `${prop}_cb`;
        if ((prop_cb in callback)
            && typeof callback[prop_cb] == "function") {
                callback[prop_cb](value. oldValue);
        } else {
            callback(prop, value, oldValue);
        }
      }
      return true;
    }
  });
}
