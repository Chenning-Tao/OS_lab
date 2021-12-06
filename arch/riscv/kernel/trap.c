#include "printk.h"
#include "clock.h"

// unsigned long count = 0;

void trap_handler(unsigned long scause, unsigned long sepc){
    if (scause & 0x8000000000000000){
        unsigned long temp = scause & 0x7fffffffffffffff;
        if(temp == 5){
            clock_set_next_event();
        }
    }
}