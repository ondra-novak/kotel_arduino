//@require temp_gauge.js
//@require tray.js
//@require control_box.js
//@require netstat.js
//@require protocol.js
//@require status.js
//@require config_therm.js
//@require tray_config.js
//@require bind_device.js
//@require header_status.js

/// <reference path="./temp_gauge.js"/>
/// <reference path="./tray.js"/>
/// <reference path="./control_box.js"/>
/// <reference path="./netstat.js"/>
/// <reference path="./protocol.js"/>
/// <reference path="./status.js"/>
/// <reference path="./config_therm.js"/>
/// <reference path="./tray_config.js"/>
/// <reference path="./bind_device.js"/>
/// <reference path="./header_status.js"/>



function main() {
    const f = FormView.load("dashboard");
    f.mount();
    window.test_form = f;
}