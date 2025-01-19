//MJ_kg - vyhrevnost [MJ/kg]
//s_kg - rychlost dopravniku t/n [s/kg]
//pw - power [kW]
//ct - cycle time [s]

const combustion_eff = 0.894;
//result is active time at[s] 
function calculate_fuel_transport_time(MJ_kg, s_kg, pw, ct) {
    return pw/(MJ_kg*1000 * combustion_eff)*s_kg*ct;   // [kJ/s * kg /kJ * s / kg * s] = [s])]
} 

//result is power [kJ/s = kW])
function calculate_power(MJ_kg, s_kg, ct, at) {
    return (MJ_kg * 1000 * combustion_eff) / (s_kg * ct ) * at; //[kJ/kg * kg/s2 * s] = [kJ/s] = [kW]
}

function calc_feeder_speed(time, initial_fill_kg) {
    if (isNaN(initial_fill_kg) || isNaN(time) || !initial_fill_kg) return 240;   //fallback value
    return time/initial_fill_kg;
}
function calculateCycleParams(MJ_kg, s_kg, pw, f) {
    let berr = 1;
    let sel = 0;
    let ct = 0;
    let at = 0;
    for (let a = 5; a < 20; ++a) {
        ct = Math.round(a/f);        
        let pw2 = calculate_power(MJ_kg,s_kg, ct, a);
        let err = Math.abs(pw2 - pw);
        if (err < berr) {
            berr = err;
            sel = a;
        }
    }
    ct = sel/f;
    at = sel;
    console.log(berr);
    return [at,ct];
}
