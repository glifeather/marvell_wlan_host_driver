#include "systime.h"

static volatile uint32_t sys_time = 0;

void SysTick_Handler(void) { ++sys_time; }

uint32_t sys_now(void) { return sys_time; }
