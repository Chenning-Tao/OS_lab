.extern start_kernel

    .section .text.init
    .globl _start
_start:
# init stack
    la sp, boot_stack_top
# set stvec = _traps
    addi sp, sp, -8
    sw t0, 0(sp)        # save t0
    la t0, _traps
    csrw stvec, t0
# set sie[STIE] = 1
    csrr t0, sie
    ori t0, t0, 0x00000020
    csrw sie, t0
# set first time interrupt
    rdtime t0
    li t1, 0x989680
    add t0, t0, t1
    li t1, 0x54494D45
    mv a7, t1
    mv a0, t0
    mv a1, x0
    mv a2, x0
    mv a3, x0
    mv a4, x0
    mv a5, x0
    mv a6, x0
    ecall

# set sstatus[SIE] = 1
    csrr t0, sstatus
    ori t0, t0, 0x00000002
    csrw sstatus, t0
# restore
    lw t0, 0(sp)
    addi sp, sp, 8
    # ------------------
    # - your code here -
    # ------------------
    call mm_init
    call task_init
    call start_kernel
    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096

    .globl boot_stack_top
boot_stack_top: