.extern dummy
.extern struct task_struct* current
    .section .text.entry
    .align 2
    .global _traps
    .global __dummy
    .global __switch_to
__switch_to:
    # save state to prev process
    sd sp, 0x30(a0)
    sd ra, 0x28(a0)
    sd s0, 0x38(a0)
    sd s1, 0x40(a0)
    sd s2, 0x48(a0)
    sd s3, 0x50(a0)
    sd s4, 0x58(a0)
    sd s5, 0x60(a0)
    sd s6, 0x68(a0)
    sd s7, 0x70(a0)
    sd s8, 0x78(a0)
    sd s9, 0x80(a0)
    sd s10, 0x88(a0)
    sd s11, 0x90(a0)
    csrr t0, sepc
    sd t0, 0x98(a0)
    csrr t0, sstatus
    sd t0, 0xa0(a0)
    csrr t0, sscratch
    sd t0, 0xa8(a0)
    # restore state from next process
    ld t0, 32(a1)
    ld sp, 0x30(a1)
    ld ra, 0x28(a1)
    ld s0, 0x38(a1)
    ld s1, 0x40(a1)
    ld s2, 0x48(a1)
    ld s3, 0x50(a1)
    ld s4, 0x58(a1)
    ld s5, 0x60(a1)
    ld s6, 0x68(a1)
    ld s7, 0x70(a1)
    ld s8, 0x78(a1)
    ld s9, 0x80(a1)
    ld s10, 0x88(a1)
    ld s11, 0x90(a1)
    ld t0, 0x98(a1)
    csrw sepc, t0
    ld t0, 0xa0(a1)
    csrw sstatus, t0
    ld t0, 0xa8(a1)
    csrw sscratch, t0
    # 切换页表
    ld t0, 0xb0(a1)
    srli t0, t0, 12
    li t1, (8 << 60)
    or t0, t0, t1
    csrw satp, t0
    sfence.vma zero, zero
    ret
_traps:
save_register:
# # 保存会用到的寄存器的值
#     addi sp, sp, -24
#     sd t0, 0(sp)
#     sd t1, 8(sp)
#     sd t2, 16(sp)
# 获取目前线程的地址
    la t0, current
    ld t1, 0(t0)
# 切换Mode, t2为sscratch
    ld t2, 0xa8(t1)
    beq t2, x0, no_switch1
    sd t2, 0x30(t1)
    sd sp, 0xa8(t1)
    mv sp, t2
no_switch1:
    addi sp, sp, -256
    sd x1, 0(sp)
    sd x3, 8(sp)
    sd x4, 16(sp)
    sd x5, 24(sp)
    sd x6, 32(sp)
    sd x7, 40(sp)
    sd x8, 48(sp)
    sd x9, 56(sp)
    sd x10, 64(sp)
    sd x11, 72(sp)
    sd x12, 80(sp)
    sd x13, 88(sp)
    sd x14, 96(sp)
    sd x15, 104(sp)
    sd x16, 112(sp)
    sd x17, 120(sp)
    sd x18, 128(sp)
    sd x19, 136(sp)
    sd x20, 144(sp)
    sd x21, 152(sp)
    sd x22, 160(sp)
    sd x23, 168(sp)
    sd x24, 176(sp)
    sd x25, 184(sp)
    sd x26, 192(sp)
    sd x27, 200(sp)
    sd x28, 208(sp)
    sd x29, 216(sp)
    sd x30, 224(sp)
    sd x31, 232(sp)
    csrr t0, sepc
    sd t0, 240(sp)
    sd sp, 248(sp)
call_interrupt:
    csrr a0, scause
    mv a1, t0
    mv a2, sp
    call trap_handler
load_register:
    ld x1, 0(sp)
    ld x3, 8(sp)
    ld x4, 16(sp)
    ld x5, 24(sp)
    ld x6, 32(sp)
    ld x7, 40(sp)
    ld x8, 48(sp)
    ld x9, 56(sp)
    ld x10, 64(sp)
    ld x11, 72(sp)
    ld x12, 80(sp)
    ld x13, 88(sp)
    ld x14, 96(sp)
    ld x15, 104(sp)
    ld x16, 112(sp)
    ld x17, 120(sp)
    ld x18, 128(sp)
    ld x19, 136(sp)
    ld x20, 144(sp)
    ld x21, 152(sp)
    ld x22, 160(sp)
    ld x23, 168(sp)
    ld x24, 176(sp)
    ld x25, 184(sp)
    ld x26, 192(sp)
    ld x27, 200(sp)
    ld x28, 208(sp)
    ld x29, 216(sp)
    ld x30, 224(sp)
    ld x31, 232(sp)
    ld t0, 240(sp)
    addi t0, t0, 4
    csrw sepc, t0
    addi sp, sp, 256
# 获取目前线程的地址
    la t0, current
    ld t1, 0(t0)
# 切换Mode, t2为sscratch
    ld t2, 0xa8(t1)
    beq t2, x0, no_switch2
    sd t2, 0x30(t1)
    sd sp, 0xa8(t1)
    mv sp, t2
no_switch2:
    # ld t0, 0(sp)
    # ld t1, 8(sp)
    # ld t2, 16(sp)
    # addi sp, sp, 24
    sret

__dummy:
# 获取目前线程的地址
    la t0, current
    ld t1, 0(t0)
# 切换Mode, t2为sscratch
    ld t2, 0xa8(t1)
    beq t2, x0, no_switch2
    sd t2, 0x30(t1)
    sd sp, 0xa8(t1)
    mv sp, t2

    la t0, dummy
    csrw sepc, x0
    sret