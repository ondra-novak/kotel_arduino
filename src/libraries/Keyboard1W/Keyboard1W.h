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
    uint16_t level;
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
    ///user state flag
    uint16_t _user_state:1;
    ///contains timestamp (/16, modulo 4096) when last change has been detected
    uint16_t _tp:12;


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

    ///Sets user state
    /** user state is user controlled state flag, which can be set to 1 or 0
     *
     * This flag can be used, for example, to indicate a long press.
     * If the user holds down the key, then it's a good idea to set this flag.
     * When the key is released, this flag can be used to determine whether
     * a long press occurred. If this flag is not set, then it was
     * a short press. However, the meaning of this flag is not tied
     * to this functionality and can be used for any purpose.
     *
     * */
    void set_user_state() {
        _user_state = true;
    }

    ///Tests the user state and resets its
    /**
     * @return value of user state
     *
     * @note function also resets the state, so next call returns false until the
     * flag is set by set_user_state()
     */
    bool test_and_reset_user_state() {
        if (_user_state) {
            _user_state = false;
            return true;
        }
        return false;
    }
  };


  ///Contains keyboard state
  struct State {
    KeyState _states[_nkeys] = {};
    uint16_t last_level;
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
      int d = abs(static_cast<int>(def->level) - v);
      if (d < bdff) {
        bdff = d;
        sel = def;
      }
    }

    if (sel->level == state.last_level) {

        for (int i = 0; i < _nkeys; ++i) {
          KeyState &st = state._states[i];
          auto cur = st._pressed != 0;
          auto nst = sel->keys[i];
          st._changed = cur != nst?1:0;
          st._pressed = nst?1:0;
          if (st._changed) {
            st._unstable = 1;
            st._tp = get_tp(millis());
          }
        }
    } else {
        for (int i = 0; i < _nkeys; ++i) {
            KeyState &st = state._states[i];
            st._changed = 0;
        }
        state.last_level = sel->level;
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

