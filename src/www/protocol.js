const  WsReqCmd = {
    control_status : 'c',
    set_fuel : 'f',
    get_config : 'C',
    set_config : 'S',
    failed_config : 'F',
    get_stats : 'T',
    ping : 'p',
    enum_tasks : '#',
    generate_code : 'G',
    unpair_all :'U',
    reset : '!',
    clear_stats : '0',
    monitor_cycle : 'm'
};

function parseResponse(data) {
    return Object.fromEntries(
        data.split("\n")
        .map(x=>x.split('=',2)));
}

function buildRequest(params) {
   return new URLSearchParams(params).toString();
}