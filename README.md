# Řízení kotle Arduinem

Toto je víceméně soukromý projekt "řízení kotle Arduinem"

Konkrétně jde o kotel Benekov Pelling 27 poloautomatický kotel emisní třídy 3 s předepsaným palivem dřevěné pelety, agropelety nebo obilí

Řízení probíhá formou ovládání tří zařízení
- čerpadlo (pump)
- podavač (feeder)
- vetilátor (fan)

Vstupem do řídícího procesu jsou
- výstupní teplota vody - teploměr umístěn na vnitřní nádrži nebo co nejblíž k výstupnu z nádrže
- vstupní teplota vody - teploměr umístěn na vratce 
- čidlo přehrátí podavače (teplotní bimetalový čidlo, které se rozepne při překročení kritické teploty podavače)
- čidlo otevření zásobníku (spíná když se otevře zásobník s palivem)

## Board

Program je určen pro Arduino UNO R4 Wifi  s procesorem Renesas, koprocesorem ESP32 a dotmatrix maticí 12x8

## Specifické požadavky na řízení

- všechna zařízení se spouští logickou nulou (LOW)
- ventilátor má řízení otáček pomocí pomalého PWM. Frekvence PWM je cca 5-10Hz, nejkratší puls by měl být delší než půlvna 50Hz sinusu - tedy 10ms. Ovládání otáček pomocí parametru 10%-100%
- podavač se zpravidla spouští na dobu do 10s, při vyšším výkonu nebo slabé výhřevnosti může jít až na 20s


## Okrajové provozní podmínky

- vstupní teplota by neměla klesnout pod 60 stupňů (lze nastavit)
- výstupní teplota by neměla překročit 85 stupňů (lze nastavit, doporučuju nejít na 90)

## Režimy práce

- ruční ovládání - uživatel zapíná a vypíná všechna tři zařízení ručně.
- automatický režim 
    - plný výkon (profil 1)
    - snížený výkon (profil 2)
    - útlum - vypnuto vše kromě čepradla
- režim STOP - kdy se kotel dostane mimo bezpečné provozní podmínky - příliš vysoká teplota, nebo příliš nízká teplota. 
- režim otevřený zásobník - při otevření zásobníku se zastavuje podavač a ventilátor a pokračuje se po zavření zásobníku

V automatickém režimu kotel cyklicky přikládá (pouští podavač) s nastavenou prodlevou kdy se čeká na prohoření paliva. Poměr mezi dobou přikládání a prodlevou určuje výkon kotle. 

Profil 1 - plný výkon se aktivuje pokud teploty nedosahují provozního nastavení - nebo je detekován klesající trend pod provozní nastavení

Profil 2 - snížený výkon se aktivuje pokud teploty jsou v provozním rozsahu

Útlum  se aktivuje pokud trend vývoje teplot přesahuje provozní rozsah. 

Režim STOP - kotel je zastaven. Pokud je zastaven pro vysokou teplotu, čerpadlo zůstává spuštěno. V případě nízké teploty lze režim překonat přepnutím na ruční ovládání a zpět, kdy se kotel spustí v režimu zátopu, tedy jde o plný výkon bez kontroly na nízkou teplotu po určitou dobu (1 hodina). Režim STOP se může objevit i při poruše například čidla teploměru nebo přehřátí podavače. Pokud dojde k nápravě stavu, pokračuje se posledním režimem

## Ovládání

Jednotka se ovládá přes webové rozhraní. Pro nastavení parametrů Wifi je třeba se připojit přes seriový port a do konzole napsat

```
wifi.ssid=<ssid>&wifi.password=<heslo>
```

Po restartu jednotky by mělo dojít k připojení na Wifi. Mělo by být aktivní DHCP. Na display (12x8 matice) se zobrazí přidělená IP adresa

**Autorizace** - aby bylo možné ovládat jednotku z prohlížeče, je třeba provést autorizaci. Ta se provádí ověřením fyzické dostupnosti jednotky přes zobrazení písmenného kódu na DotMatrix matici. Zadáním správné kombinace do prohlížeče dojde k autorizaci.

## Překlad v Arduino IDE

Složku `libraries` je třeba nalinkovat do stejnojmenné složky ve složce Arduino (nebo tam nalinkovat všechny podsložky). Poté stačí oteřít sketch `kotel.ino` ve složce `kotel`

## Simulace

Přeložením v Linux pomocí CMake lze spustit simulaci.

```
mkdir build
cd build
cmake ..
make all
cd ..
build/bin/emul <scénář>
```

scénář je některý soubor .script v rootu (nebo si můžete napsat vlasní). Port na kterém lze v emulaci zařízení ovládat je 8080 (zpravidla localhost:8080). V souboru index_dev.html je vývojovářská verze webové aplikace, kterou lze upravovat v Chrome (css a js)







