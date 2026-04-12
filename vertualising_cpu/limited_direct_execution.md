# **Limited Direct Execution:**
- give the full controll of the cpu to each process with some restrictions for some time interval
- **Two points:** ` limit what it can do ` & ` os must be able to stop it in b/w and switch to a different program which it is running `
- to fullfill this requirment os needs some hardware support

# **Trap Table — Under the Hood 🔧**

> **日本語:** "罠" (*wana*) = Trap — but in OS context it means "intentional jump into the kernel"

---

## First — the two modes, physically

This is not software — this is **actual hardware**, a physical bit inside the CPU chip called the **mode bit**:

```
CPU mode bit = 0  →  kernel mode  (full access to everything)
CPU mode bit = 1  →  user mode    (restricted, cannot touch hardware directly)
```

When your program runs, that bit is `1`. When the OS runs, that bit is `0`. The CPU checks this bit on **every single instruction** it executes.

```
your program tries to write directly to a hardware port
    |
CPU checks mode bit → 1 (user mode)
    |
CPU refuses and throws an exception
    |
OS kills your program
```

> **日本語:** "保護" (*hogo*) = Protection — the mode bit is pure hardware 保護

---

## The problem this creates

If user programs cannot touch hardware — how does your program ever do anything useful? Reading a file, printing to screen, sending a network packet — all of these need hardware access.

The answer is — **your program cannot do it directly, it has to ask the OS to do it**. That asking mechanism is the **system call**, and the physical mechanism that makes it work is the **trap instruction**.

---

## What actually happens during a system call

Say your program calls `read()` to read a file:

```
your code calls read()
    |
    | ← you think you are calling a function
    |    but really you are triggering a trap instruction
    ↓
CPU sees trap instruction
    |
CPU does 3 things ATOMICALLY (all at once, cannot be interrupted):
    1. saves your current registers onto the kernel stack
    2. flips mode bit from 1 → 0  (now in kernel mode)
    3. jumps to a fixed address — the TRAP HANDLER in the OS
    |
    ↓
OS kernel is now running (mode bit = 0, full access)
    |
OS looks up the trap table to find what to do
    |
OS does the actual work (reads the file from disk)
    |
OS calls return-from-trap instruction
    |
CPU does 3 things ATOMICALLY again:
    1. restores your saved registers
    2. flips mode bit from 0 → 1  (back to user mode)
    3. jumps back to your code right after where trap was called
    |
    ↓
your program continues, file data is now in your buffer
```

> **日本語:** "原子的" (*genshiteki*) = Atomic — happens all at once, cannot be split or interrupted

---

## Now — what IS the trap table exactly?

Here is the key question — when the CPU jumps into the kernel, **how does it know WHERE to jump**?

The OS cannot let user programs decide where in the kernel to jump — that would be a massive security hole:

```
// if programs could choose where to jump
// attacker could jump to:
jump to kernel address 0x4050  // skip the permission check!
jump to kernel address 0x8020  // jump straight to "format disk"!
```

So instead — **at boot time**, before any user program ever runs, the OS fills a table in memory called the **trap table**:

```
TRAP TABLE (set up at boot, in kernel memory)
┌─────────────────┬──────────────────────────────┐
│ Trap Number     │ Address of handler in kernel  │
├─────────────────┼──────────────────────────────┤
│ 0 (read)        │ 0xFFFF1000  ← kernel read()  │
│ 1 (write)       │ 0xFFFF2000  ← kernel write() │
│ 2 (fork)        │ 0xFFFF3000  ← kernel fork()  │
│ 3 (open)        │ 0xFFFF4000  ← kernel open()  │
│ 14 (timer)      │ 0xFFFF5000  ← timer handler  │
│ ...             │ ...                           │
└─────────────────┴──────────────────────────────┘
```

Then the OS tells the hardware **exactly where this table lives** in memory using a special privileged instruction — and after that the OS locks it down. User programs can never modify this table.

> **日本語:** "起動時" (*kidouji*) = At boot time — the trap table is set up at 起動時 and never touched again by user programs

---

## How the system call number works

When your program wants to call `read()` — it does not jump anywhere itself. It just puts a **number** into a register and fires the trap:

```
// what the C library's read() actually does under the hood:

mov  eax, 0        // system call number for read = 0
mov  ebx, fd       // argument 1 — file descriptor
mov  ecx, buffer   // argument 2 — where to put data
mov  edx, size     // argument 3 — how much to read
int  0x80          // fire the trap instruction!
//   ^^^^
//   this is the actual trap instruction on x86
//   "software interrupt 0x80"
```

CPU receives the trap, looks at register `eax` → sees `0` → looks up trap table entry `0` → jumps to the kernel's read handler. The user program had **zero control** over where the jump went — only the number.

---

## Full picture together

```
BOOT TIME:
OS fills trap table → tells CPU where table is → locks it

─────────────────────────────────────────────────────────

RUNTIME:

user program                CPU              OS kernel
     |                       |                    |
  calls read()               |                    |
     |                       |                    |
  puts 0 in eax              |                    |
     |                       |                    |
  fires INT 0x80 ───────────>|                    |
                        saves registers           |
                        flips mode bit 1→0        |
                        looks up trap table[0]───>|
                                             executes read
                                             reads from disk
                                             puts data in buffer
                                                  |
                        return-from-trap <─────────|
                        restores registers         |
                        flips mode bit 0→1         |
     |<──────────────────────|                    |
  continues running          |                    |
  data is in buffer          |                    |
```

---

## 🗾 Japanese Recap

| Japanese | Romaji | Meaning |
|---|---|---|
| 罠 | *wana* | Trap |
| 保護 | *hogo* | Protection |
| 原子的 | *genshiteki* | Atomic |
| 起動時 | *kidouji* | At boot time |
| 特権 | *tokken* | Privilege |
| 切り替え | *kirikae* | Switch (mode switch!) |

The trap table is essentially a **contract between hardware and OS** — set up once at boot, never changeable by anyone else, the only legal gateway from user land into the kernel. 🎯

Pretty much yes — but there are **2 small things** that naturally extend from this that OSTEP will hit right after:

---

## 1. Timer Interrupt

Right now one question is still unanswered:

> **"What if a user program never calls a system call? It just runs an infinite loop — how does the OS ever get control back?"**

```c
while(1) {
    // no system call ever
    // OS is stuck, can never run
    // how does scheduler ever kick in?
}
```

The answer is the **timer interrupt** — a hardware clock that fires every few milliseconds, forcefully traps into the kernel even if the program never asked. This is how the OS **takes** the CPU back, not waits for the program to give it back.

> **日本語:** "強制" (*kyousei*) = Forceful — the timer is a 強制 trap

---

## 2. Context Switch

Once the timer fires and OS has control — it needs to actually **save one process and load another**. That saving/loading is the context switch, which uses exactly the same register saving mechanism you just learned from traps.

---

Both of these are literally the next few paragraphs in OSTEP — so you will hit them immediately. The trap table concept you just learned is the foundation for both.

Shall we keep going? 🚀