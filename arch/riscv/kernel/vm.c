#include "defs.h"
#include "types.h"
#include "printk.h"
#include "vm.h"
/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
unsigned long early_pgtbl[512] __attribute__((__aligned__(0x1000)));
/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));
extern unsigned long long _stext;
extern unsigned long long _srodata;
extern unsigned long long _sdata;
extern unsigned long long _sbss;
extern unsigned long long _ekernel;

void setup_vm(void) {
    /* 
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表 
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。 
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */
    // 0x8000_0000 --> 0x8000_0000
    early_pgtbl[2] = (0x80000 << 10) | 0xcf;
    // 0x8000_0000 --> 0xffff_ffe0_0000_0000
    // 高9位 110000000
    early_pgtbl[384] = (0x80000 << 10) | 0xcf;
}

void setup_vm_final(void) {
    memset(swapper_pg_dir, 0x0, PGSIZE);
    // No OpenSBI mapping required

    // mapping kernel text X|-|R|V
    create_mapping(swapper_pg_dir, (uint64)&_stext, (uint64)&_stext - PA2VA_OFFSET, (uint64)&_srodata - (uint64)&_stext, 11);
       
    // mapping kernel rodata -|-|R|V
    create_mapping(swapper_pg_dir, (uint64)&_srodata, (uint64)&_srodata - PA2VA_OFFSET, (uint64)&_sdata - (uint64)&_srodata, 3);
    
    // mapping other memory -|W|R|V
    create_mapping(swapper_pg_dir, (uint64)&_sdata, (uint64)&_sdata - PA2VA_OFFSET, PGSIZE, 7);
    create_mapping(swapper_pg_dir, (uint64)&_sbss, (uint64)&_sbss - PA2VA_OFFSET, PHY_END - ((uint64)&_sbss - PA2VA_OFFSET), 7);

    __asm__ volatile (
        "mv t0, %[addr]\n"
        "srli t0, t0, 12\n"
        "li t1, (8 << 60)\n"
        "or t0, t0, t1\n"
        "csrw satp, t0\n"
        :
        : [addr] "r" ((uint64)&swapper_pg_dir - PA2VA_OFFSET)
        : "memory"
    );
    // flush TLB
    asm volatile("sfence.vma zero, zero");
    return;
}


/* 创建多级页表映射关系 */
create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm) {
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小
    perm 为映射的读写权限

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */
    uint64 va_current, pa_current;
    uint64 *cur_pgtbl, tmp_pgtbl;
    for (va_current = va, pa_current = pa; va_current < va + sz; va_current += PGSIZE, pa_current += PGSIZE){
        uint64 vn2 = (va_current >> 30) & 0x1ff;
        uint64 vn1 = (va_current >> 21) & 0x1ff;
        uint64 vn0 = (va_current >> 12) & 0x1ff;
        cur_pgtbl = pgtbl;
        uint64 phyaddr;
        // 第2级
        // 如果需要创建子页表
        if (!(cur_pgtbl[vn2] & 0x1)) {
            tmp_pgtbl = kalloc();
            phyaddr = (uint64)tmp_pgtbl - PA2VA_OFFSET;
            cur_pgtbl[vn2] = (uint64)((uint64)((phyaddr >> 12) << 10) | 0x1);
            cur_pgtbl = tmp_pgtbl;
        } else {
            cur_pgtbl = (cur_pgtbl[vn2] >> 10) << 12;
        }
        // 第1级
        // 如果需要创建子页表
        if (!(cur_pgtbl[vn1] & 0x1)) {
            tmp_pgtbl = kalloc();
            phyaddr = (uint64)tmp_pgtbl - PA2VA_OFFSET;
            cur_pgtbl[vn1] = (uint64)((uint64)((phyaddr >> 12) << 10) | 0x1);
            cur_pgtbl = tmp_pgtbl;
        } else {
            cur_pgtbl = (cur_pgtbl[vn1] >> 10) << 12;
        }
        // 第0级
        cur_pgtbl[vn0] = (uint64)((pa_current >> 12) << 10) + perm;
    }

}