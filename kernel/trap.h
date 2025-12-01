#ifndef TRAP_H
#define TRAP_H

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "proc.h"
#include "spinlock.h"

// Initialize trap handling
void trapinit(void);
void trapinithart(void);

// Trap handling entry points
void kernelvec(void);
void usertrapret(void);
void kerneltrap(void);

// Page fault and lazy loading helpers
void handle_page_fault(uint64 scause, uint64 stval);

// Utility
int copy_trapframe(struct proc *p, struct trapframe *tf);

#endif // TRAP_H
