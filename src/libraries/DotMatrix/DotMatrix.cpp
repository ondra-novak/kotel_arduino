#include "../DotMatrix/DotMatrix.h"

#include <Arduino.h>
#include <FspTimer.h>

namespace DotMatrix {

static constexpr unsigned int pin_zero_index = 28;


namespace DirectDrive {

static constexpr uint32_t LED_MATRIX_PORT0_MASK  = ((1 << 3) | (1 << 4) | (1 << 11) | (1 << 12) | (1 << 13) | (1 << 15));
static constexpr uint32_t LED_MATRIX_PORT2_MASK  = ((1 << 4) | (1 << 5) | (1 << 6) | (1 << 12) | (1 << 13));
void clear_matrix()  {
  R_PORT0->PCNTR1 &= ~LED_MATRIX_PORT0_MASK;
  R_PORT2->PCNTR1 &= ~LED_MATRIX_PORT2_MASK;
}
void activate_row(int row, bool high)  {
  bsp_io_port_pin_t pin_a = g_pin_cfg[row + pin_zero_index].pin;
  R_PFS->PORT[pin_a >> 8].PIN[pin_a & 0xFF].PmnPFS =
    IOPORT_CFG_PORT_DIRECTION_OUTPUT | (high?IOPORT_CFG_PORT_OUTPUT_HIGH:IOPORT_CFG_PORT_OUTPUT_LOW);
}
void deactivate_row(int row)  {
  bsp_io_port_pin_t pin_a = g_pin_cfg[row + pin_zero_index].pin;
  if (pin_a >> 8) {
      R_PORT2->PCNTR1 &= ~(1 << (pin_a & 0xFF));
  } else {
      R_PORT0->PCNTR1 &= ~(1 << (pin_a & 0xFF));
  }
}



}


class AutoDriveTimer {
public:

    static AutoDriveTimer *instance;

    void set_freq(unsigned int freq, const TimerFunction &cb) {
        if (freq != _freq || cb != _cb) {
            if (_freq) {
                _timer.stop();
                _timer.close();
            }
            if (freq && cb) {
                uint8_t timer_type = GPT_TIMER;
                int8_t tindex = FspTimer::get_available_timer(timer_type);
                if (tindex < 0){
                    tindex = FspTimer::get_available_timer(timer_type, true);
                }
                _freq = freq;
                _timer.begin(TIMER_MODE_PERIODIC, timer_type, tindex, freq, 0.0f,
                        [](auto){instance->_cb();});
                _timer.setup_overflow_irq();
                _timer.open();
                _timer.start();
            }
            _cb = cb;
            _freq = freq;
        }
    }

protected:
    FspTimer _timer;
    TimerFunction _cb;
    unsigned int _freq = 0;
};

AutoDriveTimer *AutoDriveTimer::instance = nullptr;

void enable_auto_drive(TimerFunction cb, unsigned int freq) {
    if (AutoDriveTimer::instance == nullptr) AutoDriveTimer::instance = new AutoDriveTimer();
    AutoDriveTimer::instance->set_freq(freq, cb);
}

void disable_auto_drive() {
    if (AutoDriveTimer::instance == nullptr) return;
    AutoDriveTimer::instance->set_freq(0, {});
}

}

