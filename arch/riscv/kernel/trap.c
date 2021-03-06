#include "printk.h"
#include "clock.h"
#include "trap.h"
#include "syscall.h"
// unsigned long count = 0;
#include "proc.h"
extern struct task_struct* current;
extern struct task_struct* task[NR_TASKS];

void trap_handler(unsigned long scause, unsigned long sepc, unsigned long *sp){
    unsigned long long a0 = sp[8];
    unsigned long long a1 = sp[9];
    unsigned long long a2 = sp[10];
    unsigned long long a7 = sp[15];
    // printk("%lx %lx %lx %lx\n", task[1]->thread.sscratch, task[2]->thread.sscratch, task[3]->thread.sscratch, task[4]->thread.sscratch);
    if (scause & 0x8000000000000000){
        unsigned long temp = scause & 0x7fffffffffffffff;
        if(temp == 5){
            clock_set_next_event();
        }
    } else {
        switch (scause)
        {
        case 8:
            if (a7 == 172){
                sp[8] =  sys_getpid();
            }
            else if (a7 == 64){
                sys_write(a0, a1, a2);
            }
            break;
        default:
            break;
        }
    }
}