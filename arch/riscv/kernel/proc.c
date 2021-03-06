//arch/riscv/kernel/proc.c

#include "proc.h"
#include "defs.h"
#include "vm.h"

extern void __dummy();
extern void __switch_to(struct task_struct* prev, struct task_struct* next);
extern unsigned long swapper_pg_dir[512];
extern unsigned long long uapp_start;

struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组，所有的线程都保存在此

void dummy() {
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if (last_counter == -1 || current->counter != last_counter) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. thread space begin at %lx, sp %lx\n", current->pid, (uint64)current, current->thread.sp);
        }
    }
}

void task_init() {
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    idle = (struct task_struct*) kalloc();
    // 2. 设置 state 为 TASK_RUNNING;
    idle->state = TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    idle->priority = 0;
    idle->counter = 0;
    // 4. 设置 idle 的 pid 为 0
    idle->pid = 0;
    // 5. 将 current 和 task[0] 指向 idle
    current = idle;
    task[0] = idle;
    

    /* YOUR CODE HERE */

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, counter 为 0, priority 使用 rand() 来设置, pid 为该线程在线程数组中的下标。
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`,
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址， `sp` 设置为 该线程申请的物理页的高地址

    /* YOUR CODE HERE */
    for(int i = 1; i <= NR_TASKS - 1; ++i){
        task[i] = (struct task_struct*) kalloc();
        task[i]->state = TASK_RUNNING;
        task[i]->counter = 0;
        task[i]->priority = rand();
        task[i]->pid = i;
        task[i]->thread.ra = (void *)__dummy;
        task[i]->thread.sp = (uint64)task[i] + PGSIZE;
        // U-Mode Stack
        uint64 U_stack = kalloc();
        // 获取一页作为用户页表
        uint64 pgb_addr = kalloc();
        unsigned long *pgtbl = (unsigned long *)pgb_addr;
        // 将系统页表放入用户页表中
        for(int i = 0; i < 512; ++i){
            pgtbl[i] = swapper_pg_dir[i];
        }
        // 将用户程序放入用户页表中
        // -|U|X|W|R|V
        create_mapping(pgtbl, USER_START, (uint64)&uapp_start - PA2VA_OFFSET, PGSIZE, 31);
        // U-Mode Stack放入用户页表中
        // -|U|X|W|R|V
        create_mapping(pgtbl, USER_END - PGSIZE, U_stack - PA2VA_OFFSET, PGSIZE, 31);
        // 保存page table地址
        task[i]->pgd = pgb_addr - PA2VA_OFFSET;
        // 修改sepc
        task[i]->thread.sepc = USER_START;
        // 修改sstatus
        __asm__ volatile (
            "csrr t0, sstatus\n"
            "li t1, 0x00040020\n"
            "or t0, t0, t1\n"
            "mv %[sstatus], t0\n"
            : [sstatus] "=r" (task[i]->thread.sstatus)
            : 
            : "memory"
        );
        // 修改sscratch
        task[i]->thread.sscratch = USER_END;
    }
    
    printk("...proc_init done!\n");
}

void switch_to(struct task_struct* next) {
    /* YOUR CODE HERE */
    struct task_struct* temp = current;
    if(next != current) {
        current = next;
        __switch_to(temp, next);
    }
}

void do_timer(void) {
    /* 1. 如果当前线程是 idle 线程 直接返回 */
    /* 2. 如果当前线程不是 idle 对当前线程的运行剩余时间减 1 
          若剩余时间任然大于0 则直接返回 否则进行调度 */
    if(current == idle) schedule();
    else if(current != idle){
        current->counter--;
        if(current->counter == 0) schedule();
    }
    else printk("idle process is running!\n");
    // schedule();
}

#ifdef NONE
int cur = 0;
void schedule(void) {
    if (cur == NR_TASKS-1) cur = 1;
    else cur = (cur + 1) % NR_TASKS;
    switch_to(task[cur]);
}
#endif

#ifdef SJF
void schedule(void) {
    int min_value = 999999;
    int min = -1;
    // 判断是否全部为0
    int reset = 1;
    for (int i = 1; i <= NR_TASKS - 1; ++i){
        if(task[i]->counter != 0) {
            reset = 0;
            break;
        }
    }
    // 如果全部为0 reset
    if (reset){
        for (int i = 1; i <= NR_TASKS - 1; ++i){
            task[i]->counter = rand();
            printk("SET [PID = %d COUNTER = %d]\n", task[i]->pid, task[i]->counter);
        }
    }
    for(int i = 1; i <= NR_TASKS - 1; ++i){
        if(task[i]->counter == 0) continue;
        if(task[i]->counter < min_value) {
            min = i;
            min_value = task[i]->counter;
        }
        else if(task[i]->counter == min_value) {
            if(task[i]->counter > task[min]->counter) {
                min = i;
                min_value = task[i]->counter;
            }
        }
    }
    printk("switch to [PID = %d COUNTER = %d]\n", task[min]->pid, task[min]->counter);
    switch_to(task[min]);
}
#endif

#ifdef PRIORITY
void schedule(void) {
    // 判断是否全部为0
    int reset = 1;
    for (int i = 1; i <= NR_TASKS - 1; ++i){
        if(task[i]->counter != 0) {
            reset = 0;
            break;
        }
    }
    // 如果全部为0 reset
    if (reset){
        for (int i = 1; i <= NR_TASKS - 1; ++i){
            task[i]->counter = rand();
            printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n", task[i]->pid, task[i]->priority, task[i]->counter);
        }
    }
    printk("\n");

    int c = -1;
    int next = 0;
    for(int i = NR_TASKS-1; i > 0; --i){
        if (task[i]->counter == 0) continue;
        if (task[i]->priority > task[next]->priority)
            next = i;
        if (task[i]->priority == task[next]->priority)
            if(task[i]->counter < task[next]->counter)
                next = i;
    }
    printk("switch to [PID = %d PRIORITY = %d COUNTER = %d]\n", task[next]->pid, task[next]->priority, task[next]->counter);
    switch_to(task[next]);
}
#endif