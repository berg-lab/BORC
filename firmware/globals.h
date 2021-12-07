#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <Arduino.h>
#include "common.h"
#include "rotary_knob.h"
#include "display.h"
#include "power_manager.h"
#include "servo.h"
#include "system.h"
#include "manual_mode.h"
#include "sleep_manager.h"

extern CRotaryKnob Knob;
extern CLedMatrix Display;
extern CPowerManager PowerMgr;
extern CServoDriver Servo;
extern CSystem System;
extern CManualModeMgr ManualModeMgr;
extern CSleepMgr SleepMgr;

#endif
