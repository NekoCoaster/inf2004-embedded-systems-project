
#ifndef NFA4X10_Rebuilt_h_
#define NFA4X10_Rebuilt_h_
#include <stdio.h>
#include "pico/stdlib.h"

float NFA4X10_get_fan_rpm();
int NFA4X10_get_fan_duty_cycle();
void NFA4X10_set_fan_speed(int speed_percent);
void NFA4X10_init();

#endif
// NFA4X10_Rebuilt_h_