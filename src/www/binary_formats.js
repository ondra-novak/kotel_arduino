
const StatusOutWs = [
    ["uint32", "timestamp"],
    ["uint32", "feeder_time"],
    ["uint32", "tray_open_time"],
    ["uint32", "tray_fill_time"],
    ["int16", "tray_fill_kg"],
    ["int16", "bag_consumption"],
    ["int16", "temp_output_value"],
    ["int16", "temp_output_amp_value"],
    ["int16", "temp_input_value"],
    ["int16", "temp_input_amp_value"],
    ["int16", "rssi"],
    ["uint8", "temp_sim"],
    ["uint8", "temp_input_status"],
    ["uint8", "temp_output_status"],
    ["uint8", "mode"],
    ["uint8", "automode"],
    ["uint8", "tray_open"],
    ["uint8", "feeder_overheat"],
    ["uint8", "pump"],
    ["uint8", "feeder"],
    ["uint8", "fan"],
];

const ManualControlWs = [
    ["uint8", "feeder_timer"],
    ["uint8", "fan_timer"],
    ["uint8", "fan_speed"],
    ["uint8", "force_pump"]
];

const SetFuelWs = [
    ["int16", "kgchg"],
    ["int8", "kalib"],
    ["int8", "absnow"],
    ["int8", "full"],
    ["int8", "reserved"],
];

const StatsOutWs = [
    ["uint32", "fan_time"],
    ["uint32", "pump_time"],
    ["uint32", "full_power_time"],
    ["uint32", "low_power_time"],
    ["uint32", "cooling_time"],
    ["uint32", "active_time"],
    ["uint32", "overheat_time"],
    ["uint32", "stop_time"],
    ["uint32", "reserved1"],
    ["uint32", "reserved2"],
    ["uint32", "feeder_start_count"],
    ["uint32", "fan_start_count"],
    ["uint32", "pump_start_count"],
    ["uint16", "feeder_overheat_count"],
    ["uint16", "tray_open_count"],
    ["uint16", "start_count"],
    ["uint16", "overheat_count"],
    ["uint32", "full_power_count"],
    ["uint32", "low_power_count"],
    ["uint32", "cool_count"],
    ["uint16", "stop_count"],
    ["uint16", "temp_read_failure_count"],
    ["uint16", "reserved3"],
    ["uint16", "reserved4"],
    ["uint32", "feeder_time"],
    ["uint32", "tray_open_time"],
    ["uint32", "tray_fill_time"],
    ["uint16", "feeder_1kg_time"],
    ["uint16", "tray_fill_kg"],
    ["uint32", "consumed_kg"],
    ["uint32", "consumed_kg_total"],
    ["uint32", "eeprom_errors"],
    ["uint32", "uptime"]

];
