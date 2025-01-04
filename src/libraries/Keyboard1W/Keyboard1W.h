#pragma once

#include <stdint.h>

///Keyboard controller
/**
 * @tparam _nkeys total controlled keys
 * @tparam _nstates total valid combinations
 */
template<int _nkeys, int _nstates>
class Keyboard1W {
public:


  ///definition of signal level
  struct LevelDef {
    ///signal level value (analogRead value)
    int level;
    ///combination of keys for this level
    bool keys[_nkeys];
  };

  ///Contains state of the key
  struct KeyState {
    ///is set to 1, if is pressed
    uint16_t _pressed:1;
    ///is set to 1, if change detected (0->1 or 1->0)
    uint16_t _changed:1;
    ///is set to 1, if state is unstable (change recently)
    uint16_t _unstable:1;
    ///contains timestamp (/16, modulo 8192) when last change has been detected
    uint16_t _tp:13;


    ///determine pressed state
    /**
     * @retval true key is pressed
     * @retval false key is not pressed
     */
    bool pressed() const {return _pressed != 0;}
    ///determine change state
    /**
     * @retval true state has been changed
     * @retval false state was not changed
     */
    bool changed() const {return _changed != 0;}
    ///determine stable state
    /**
     * This flag requires to call stabilize() repeatedly
     *
     * @retval true state is stable
     * @retval false state is not stable
     */
    bool stable() const {return _unstable == 0;}
    ///tests whether key state is stable, adjusts stable flag accordingly
    /**
     * You need to call this function repeatedly until the state is stable
     * @param interval_ms specifies interval in milliseconds need to have
     * stable state to set stable flag. Maximum is 32768 ms (32 seconds)
     *
     * @retval false stable state was not changed. You need to call
     * stable() to determine current state
     * @retval true stable state was changed. The function stable() returns
     * true
     */
    bool stabilize(int16_t interval_ms) {
        if (!_unstable) return false;
        uint16_t t1 = static_cast<uint16_t>(_tp) << 4;
        uint16_t t2 = static_cast<uint16_t>(millis());
        int16_t df = t2 - t1;
        if (df > interval_ms) {
          _unstable = false;
          return true;
        }
        return false;
    }
  };


  ///Contains keyboard state
  struct State {
    KeyState _states[_nkeys] = {};
    KeyState &get_state(int key) {return _states[key];}
    const KeyState &get_state(int key) const {return _states[key];}
  };

  ///construct keyboard controller
  /**
   * @param pin analog pin to read
   * @param sdef level definitions, contains array of LevelDef, one
   * field for every state
   */
  constexpr Keyboard1W(int pin, const LevelDef (&sdef)[_nstates]):pin(pin) {
    for (int i = 0; i < _nstates; ++i) defs[i] = sdef[i];
  }

  ///initialize keybord - if constructed by constexpr constructor
  constexpr void begin() const {
    pinMode(this->pin, INPUT);
  }


  ///construct keyboard controller
  /**
   * @param sdef level definitions, contains array of LevelDef, one
   * field for every state
   */
  Keyboard1W( const LevelDef (&sdef)[_nstates]) {
    for (int i = 0; i < _nstates; ++i) defs[i] = sdef[i];
  }

  ///initialize keyboard controller
  /**
   * @param pin pin
   */
  void begin(int pin) {
    this->pin = pin;
    pinMode(pin, INPUT);
  }

  ///read pin and update state
  /**
   * @param state state object
   * @return raw value of pin. You can use this value to test connection. It
   * should return 0 when no key is pressed.
   */
  constexpr int read(State &state) const {
    int v = analogRead(pin);
    int bdff = 1024;
    const LevelDef  *sel = nullptr;

    for (int i = 0; i < _nstates; ++i) {
      const LevelDef *def = defs+i;
      int d = abs(def->level - v);
      if (d < bdff) {
        bdff = d;
        sel = def;
      }
    }

    for (int i = 0; i < _nkeys; ++i) {
      KeyState &st = state._states[i];
      uint8_t cur = st._pressed;
      uint8_t nst = sel->keys[i]?1:0;
      st._changed = cur != nst?1:0;
      st._pressed = nst;
      if (st._changed) {
        st._unstable = 1;
        st._tp = get_tp(millis());
      }
    }
    return v;
  }


protected:
  LevelDef defs[_nstates] = {};
  int pin = 0;

  static uint16_t get_tp(unsigned long milsec) {
    return static_cast<uint16_t>(milsec>>4) ;
  }

};

