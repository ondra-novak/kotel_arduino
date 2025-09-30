//@require forms.js
//@html power_config.html
//@style power_config.css


const combustion_eff = 0.894;


///result is power [kJ/s = kW])
/** 
 *  @param MJ_kg - vyhrevnost
 *  @param s_kg - rychlost podavace (s/kg)
 *  @param ct - delka cyklu v sec
 *  @param at - doba aktivity v sec
 */
function calculate_power(MJ_kg, s_kg, ct, at) {
    return (MJ_kg * 1000 * combustion_eff) / (s_kg * ct ) * at; //[kJ/kg * kg/s2 * s] = [kJ/s] = [kW]
}

function calculate_burnout(MJ_kg, s_kg, pw, at) {
    return (MJ_kg * 1000 * combustion_eff) / (s_kg * pw) * at - at;
}

function calc_consumption(speed, ft, bt) {
    return ft/(ft+bt) * 86400 / speed;
}

function powerConfig(cfgobj) {
    const f = ModalDialog.load("powerConfig");
    const cfg = cfgobj.get();
    const newcfg = {};
    const flds = f.get_fields();    
    const hval = parseFloat(cfg.hval);
    const spd = parseInt(cfg["fd.speed"]);
    flds.full_fueling = cfg["full.fueling"];
    flds.full_burnout = cfg["full.burnout"];
    flds.full_fan = cfg["full.fanpw"];
    flds.low_fueling = cfg["low.fueling"];
    flds.low_burnout = cfg["low.burnout"];
    flds.low_fan = cfg["low.fanpw"];
    const update_consuption = ()=>{
        flds.fc = calc_consumption(spd, flds.full_fueling, flds.full_burnout).toFixed(1);
        flds.lc = calc_consumption(spd, flds.low_fueling, flds.low_burnout).toFixed(1);
    }
    const update_full_power = ()=>{
        flds.full_power=calculate_power(hval, spd, flds.full_fueling+flds.full_burnout, flds.full_fueling).toFixed(0);
        update_consuption();
    }
    const update_low_power = ()=>{
        flds.low_power=calculate_power(hval, spd, flds.low_fueling+flds.low_burnout, flds.low_fueling).toFixed(0);
        update_consuption();
    }
    update_full_power();
    update_low_power();
    f.on("full_fueling","change",update_full_power);
    f.on("full_burnout","change",update_full_power);
    f.on("low_fueling","change",update_low_power);
    f.on("low_burnout","change",update_low_power);
    f.on("full_power","change",()=>{
        flds.full_fueling = Math.ceil(flds.full_power/2);     
        flds.full_burnout = Math.round(calculate_burnout(hval,spd, flds.full_power, flds.full_fueling));
        update_consuption();
    });
    f.on("low_power","change",()=>{
        flds.low_fueling = Math.ceil(flds.low_power);        
        flds.low_burnout = Math.round(calculate_burnout(hval,spd, flds.low_power, flds.low_fueling));
        update_consuption();
    });
    f.on("bok","click",async ()=>{
        newcfg["full.fueling"] = flds.full_fueling;
        newcfg["full.burnout"] = flds.full_burnout;
        newcfg["full.fanpw"] = flds.full_fan;
        newcfg["low.fueling"] = flds.low_fueling;
        newcfg["low.burnout"] = flds.low_burnout;
        newcfg["low.fanpw"] = flds.low_fan;
        await Promise.resolve(cfgobj.set(newcfg));
        f.close(f.get(["fuel","unit"]));
    })
    f.on("badv","click", ()=>flds.adv = !flds.adv);
    return f.do_modal("bst");

}