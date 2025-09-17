//@require temp_gauge.js
//@require tray.js
//@require control_box.js
//@require netstat.js
//@require protocol.js
//@require status.js
//@require config_therm.js
//@require tray_config.js
//@require bind_device.js

function main() {
    const f = FormView.load("temp_example");
    f.mount();
    window.test_form = f;
}