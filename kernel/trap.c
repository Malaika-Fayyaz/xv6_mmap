#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "trap.h"
#include "vm.h"

extern char userret[];
extern char trampoline[]; // trampoline.S
extern void kernelvec();
extern void uservec();
extern void usertrapret();

struct spinlock tickslock;
uint ticks;
int devintr(void); 

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

void
usertrap(void)
{
  int which_dev = 0;
  struct proc *p = myproc();

  if ((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // Save user program counter
  p->trapframe->epc = r_sepc();

  uint64 scause = r_scause();

  switch (scause) {
  case 8: // system call
    if (p->killed)
      panic("some error message"); 
    p->trapframe->epc += 4;
    intr_on();
    syscall();
    break;

  case 13: // page fault on load
  case 15: // page fault on store
  {
    uint64 va = r_stval();
    uint64 mem = vmfault(p->pagetable, va, scause == 15);
    if (mem == 0) {
      printf("pid %d %s: access fault va 0x%p\n", p->pid,
             scause == 15 ? "store" : "load", (void*)va);
      p->killed = 1;
    }
    break;
  }

  case 2: // illegal instruction
    printf("pid %d %s: illegal instruction at 0x%p\n", p->pid,
           p->name, (void *)r_sepc());
    p->killed = 1;
    break;

  default:
    which_dev = devintr();
    if (which_dev == 0) {
      printf("usertrap(): unexpected scause %p pid=%d\n", (void *)scause, p->pid);
      printf("sepc=%p stval=%p\n", (void *)r_sepc(), (void *)r_stval());
      p->killed = 1;
    }
    break;
  }

  if (p->killed)
     panic("some error message");

  // Give up the CPU if needed
  if (scause == 1 || scause == 5)
    yield();

  usertrapret();
}

// return to user space
void
usertrapret(void)
{
  struct proc *p = myproc();

  intr_off();

  // Set up trapframe values for uservec
  uint64 x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to go back to user mode
  x |= SSTATUS_SPIE; // enable interrupts
  w_sstatus(x);
  w_sepc(p->trapframe->epc);

  w_stvec(TRAMPOLINE + ((void *)uservec - (void *)trampoline));

  // Switch to trampoline to return to user space
  uint64 fn = TRAMPOLINE + (userret - trampoline);
  ((void(*)(void))fn)();
}

// kernel trap
void
kerneltrap()
{
  int which_dev = 0;

  if ((r_sstatus() & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");

  which_dev = devintr();
  if (which_dev == 0) {
    printf("kerneltrap(): unexpected scause %p\n", (void *)r_scause());
    panic("kerneltrap");
  }

  if (which_dev == 2)
    yield();
}

// check for device interrupt
int
devintr(void)
{
  uint64 scause = r_scause();

  if ((scause & 0x8000000000000000L) && (scause & 0xff) == 9) {
    // external interrupt from PLIC
    int irq = plic_claim();
    if (irq == UART0_IRQ)
      uartintr();
    else if (irq == VIRTIO0_IRQ)
      virtio_disk_intr();
    if (irq)
      plic_complete(irq);
    return 1;
  } else if (scause == 0x8000000000000001L) {
    // software timer interrupt
    if (cpuid() == 0) {
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    w_sip(r_sip() & ~2);
    return 2;
  } else {
    return 0;
  }
}
