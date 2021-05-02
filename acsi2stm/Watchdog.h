/* ACSI2STM Atari hard drive emulator
 * Copyright (C) 2019-2021 by Jean-Matthieu Coulon
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "acsi2stm.h"
#include <Arduino.h>

#define WATCHDOG_TIMER Timer2

// Timer-based watchdog
struct Watchdog {
  void begin(int millis = WATCHDOG_TIMEOUT);
  void feed() {
#if WATCHDOG_TIMEOUT
    WATCHDOG_TIMER.setCount(0);
#endif
  }
  void pause() {
#if WATCHDOG_TIMEOUT
    WATCHDOG_TIMER.pause();
#endif
  }
  void resume() {
#if WATCHDOG_TIMEOUT
    feed();
    WATCHDOG_TIMER.resume();
#endif
  }
protected:
  static void trigger();
};

// vim: ts=2 sw=2 sts=2 et
#endif
