//std
#include <type.h>
#include <asm.h>
// libs
#include <printk.h>
// x86
#include <pm.h>
#include <isr.h>
#include <syscall.h>
// mm
#include <vmm.h>
// proc
#include <proc.h>
#include <sysproc.h>

extern void _syscall();

int fetchint(uint32_t addr, int *ip){

    if (addr < USER_BASE || addr > USER_BASE + proc->size || addr + 4 > USER_BASE + proc->size){
        printl("fetchint: failed, addr 0x%x\n", addr);
        return -1;
    }
    *ip = *(int *)(addr);

    return 0;
}

int fetchstr(uint32_t addr, char **pp){
    char *s, *ep;

    if (addr < USER_BASE || addr >= USER_BASE + proc->size){
        printl("fetchstr: failed, addr 0x%x\n", addr);
        return -1;
    }

    *pp = (char *)addr;
    ep = (char *)(USER_BASE + proc->size);

    for (s = *pp; s < ep; s++){
        if (*s == '\0'){
            return s - *pp;
        }
    }

    return -1;
}

int argint(int n, int *ip){
    // TODO why diff wiht xv6?
    return fetchint(proc->fm->user_esp + 4*n, ip);
}

int argstr(int n, char **pp){
    int addr;

    if (argint(n, &addr) < 0){
        return -1;
    }
    
    return fetchstr((uint32_t)addr, pp);
}

static int (*sys_routines[])(void) = {
    sys_fork,
    sys_wait,
    sys_exit,
};

void sys_init(){
    idt_install(ISR_SYSCALL, (uint32_t)_syscall, SEL_KCODE << 3, GATE_TRAP, IDT_PR | IDT_DPL_USER);
}

void syscall(){
    int cn;

    printl("syscall: number: %d, form ring@%d\n", proc->fm->eax, proc->fm->cs&0x3);
    printl("syscall: stack pointer: 0x%x\n", proc->fm->user_esp);

    cn = proc->fm->eax;

    if (cn > 0 && cn < NSYSCALL && sys_routines[cn]){
        proc->fm->eax = sys_routines[cn]();

        char *arg1;
        int arg2 = 0;

        argstr(0, &arg1);
        argint(1, &arg2);

        printl("syscall: arg: %s\n", arg1);
        printl("syscall: arg: %d\n", arg2);

    } else {
        printl("syscall: no such syscall\n");
        proc->fm->eax = -1;
    }
}

