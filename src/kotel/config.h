#pragma once

namespace kotel {

struct Config {
    unsigned char _podavac_zap; //doba zapnuti podavace v sec
    unsigned char _podavac_vyp; //doba vypnuti podavace v sec
    unsigned char _fan_power; //sila ventilatoru 0-100
    unsigned char _fan_run_time; //dobeh ventilatoru
    unsigned char _max_utlum_min; //maximalni doba utlumu v minutach
    unsigned char _vstup_min_t;  //minimalni teplota na vstupu
    unsigned char _vystup_max_t;  //maximalni teplota na vystupu
    unsigned char _cerpadlo_t;   //zapinaci teplota cerpadla
};


struct Zasobnik {
    uint32_t naplneni_zas;      //cas podavace, kdy byl naplnen zasobnik
    uint32_t vyprazdneni_zas;   //cas podavace, kdy zasobnik bude vyprazdnen
    uint32_t jeden_pytel;       //spotreba jednoho pytle (v sekundach)
};

/*
 * vystup_teplota >= vystup_max_t ----> zapni utlum
 * vstup_teplota <= vstup_min_t ----> ukonci utlum
 * utlum ma prednost
 *
 *
 */


}
