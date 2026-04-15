# Mechanism: Address Translation

## The Core Idea

Every memory access a process makes uses a **virtual address**. The hardware
silently translates it to a **physical address** on every single access. The
process has zero idea this is happening.

```
virtual address (what process sees)
        +  base register
        =  physical address (where data actually lives in RAM)
```

---

## Assumptions (simplified for now)

- Each address space is contiguous in physical memory
- Address space is smaller than physical memory
- Each address space is the same size

---

## Base and Bounds — Dynamic Relocation

Two hardware registers inside the CPU's **MMU (Memory Management Unit):**

```
base register   → where in physical RAM this process starts
bounds register → size of the address space (max legal virtual address)
```

Every memory access:
```
physical address = virtual address + base

if virtual address > bounds → CPU raises exception → OS kills process
```

**Example:**
```c
process loaded at physical address 32KB → base = 32768
process accesses virtual address 128
→ physical address = 128 + 32768 = 32896   ✅; 

process accesses virtual address 4400 (address space is 4KB)
→ 4400 > 4096 → OUT OF BOUNDS → exception ☠️
```

Process thinks it lives at address 0. Reality: it lives at 32KB.
It never knows the difference — that is the illusion.

---

## Translation Example Table

```c
base = 16KB, address space size = 4KB

Virtual Address   Physical Address
──────────────────────────────────
0            →    16KB
1KB          →    17KB
3000         →    19384
4400         →    FAULT (out of bounds)
```

---

## Hardware Requirements

| Hardware | Purpose |
|---|---|
| Kernel/user mode bit | prevent user programs from touching privileged registers |
| Base + bounds registers (per CPU) | perform translation and enforce bounds on every access |
| Privileged instructions to set base/bounds | only OS can change them, not user programs |
| Exception raising | when process goes out of bounds or tries privileged operations |
| Exception handler registration | OS installs handlers at boot time via trap table |

Base and bounds registers can **only be modified in kernel mode** —
if a user process could change its own base register it could access
any memory in the system.

---

## OS Responsibilities

**On process creation:**
- Find a free slot in physical memory using the **free list**
- Load process there, set base and bounds registers
- Mark that memory as used

**On process termination:**
- Return memory to the free list
- Clean up PCB and process table entry

**On context switch:**
- Save current process's base and bounds → into its PCB
- Load next process's base and bounds ← from its PCB
- Only one base/bounds pair exists per CPU, so this must happen every switch

**On exception (out of bounds access):**
- OS exception handler runs
- Likely terminates the offending process

---

## Boot Time Setup

```
OS at boot:
    install trap handlers (illegal mem access, timer, syscall)
    initialize free list
    initialize process table
    start timer

OS starting a process:
    allocate memory, find slot in free list
    set base and bounds registers
    return-from-trap → process runs in user mode

Hardware during process execution:
    every memory access → translate virtual → physical silently
    if out of bounds → trap to OS

Context switch:
    save base/bounds of current process to PCB
    load base/bounds of next process from PCB
    return-from-trap → new process runs
```

---

## Problem — Internal Fragmentation

Base and bounds allocates a **fixed size slot** for each process:

```
process uses:   code (1KB) + heap (small) + stack (small)
slot size:      16KB
wasted space:   everything in between heap and stack ← internal fragmentation
```

The space inside the allocated region that is not used is just wasted.
Physical memory fills up faster than it needs to.

This is why base and bounds alone is not enough — it leads directly
to the next technique: **Segmentation**.

---

## Summary

| Concept | Meaning |
|---|---|
| Virtual address | what the process sees, starts at 0 |
| Physical address | where data actually lives in RAM |
| Base register | offset added to every virtual address |
| Bounds register | max legal virtual address, enforces protection |
| MMU | hardware unit inside CPU that does the translation |
| Free list | OS data structure tracking which physical memory slots are available |
| Internal fragmentation | wasted space inside a fixed allocated region |
| Dynamic relocation | address translation happens at runtime, process can be moved |