# **Context Switch:**

---

```cpp
hardware timer fires
        |
CPU saves user registers → kernel stack    (hardware does this)
CPU flips mode bit 1 → 0                   (hardware does this)
CPU jumps to timer handler in trap table   (hardware does this)
        |
OS runs — decides: switch or continue?
        |
        |── continue → return-from-trap → same process resumes
        |
        └── switch
                |
                OS saves current process registers → that process's PCB
                OS picks next process from ready queue
                OS loads next process registers ← from that PCB
                OS flips mode bit 0 → 1
                OS jumps to where that process left off
                        |
                new process is now running
```

> next

The one thing worth clarifying is the **two saves** that happen during a context switch — beginners often miss this:

| Step | Who does it | Saved where |
|---|---|---|
| Save user registers on trap | **hardware** | kernel stack |
| Save kernel registers on switch | **OS** | process PCB |

First the hardware saves you into the kernel, then the OS saves the kernel state into the PCB. Two separate saves, two separate locations. That is the part most people miss. 

Everything else you described is spot on — you are ready to move forward! 

---

Great question — this confuses almost everyone. Let me build it from scratch:

---

## First — What is a Stack?

A stack is just a **region of memory** that grows and shrinks as functions call each other:

```
void c() { ... }
void b() { c(); }
void a() { b(); }

main() { a(); }
```

```
STACK MEMORY
┌──────────────┐ ← stack grows downward
│   main()     │
│   a()        │
│   b()        │
│   c()        │ ← stack pointer (esp) points here
└──────────────┘
```

Every function call **pushes** a frame onto the stack — local variables, return address, saved registers. When function returns it **pops** back off.

> **日本語:** "積む" (*tsumu*) = To stack/pile up

---

## Every Process has TWO stacks

This is the key thing OSTEP does not say loudly enough:

```
PROCESS MEMORY LAYOUT
┌─────────────────┐
│   code          │
│   globals       │
│   heap          │
│                 │
│   USER STACK    │ ← used when your code runs (user mode)
│                 │
│   KERNEL STACK  │ ← used when OS runs ON BEHALF of this process
└─────────────────┘
```

The kernel stack is **not the OS's own memory** — it belongs to the process but only the kernel can touch it. Every process gets one.

> **日本語:** "所有" (*shoyuu*) = Ownership — the kernel stack is owned by the process but only accessible in kernel mode

---

## Now your exact question — where does hardware save registers?

```
timer fires / system call happens
        |
hardware saves user registers → THIS PROCESS'S KERNEL STACK
        |                        ^^^^^^^^^^^^^^^^^^^^^^^^^^^
        |                        NOT the PCB
        |                        NOT some global OS memory
        |                        the kernel stack of whichever
        |                        process was running
        |
flips to kernel mode
        |
OS runs using that same kernel stack
```

So yes — **hardware saves to the kernel stack of the process**, not the PCB directly.

---

## Then what does the OS save to the PCB?

When OS decides to context switch — it needs to save enough info so it can **come back to this process later**. At this point the process is already in kernel mode, so what gets saved to PCB is the **kernel stack pointer** and a few kernel registers:

```
hardware saves:
    user registers (eip, esp, eax...) → process kernel stack

OS saves:
    kernel stack pointer               → process PCB
    kernel registers                   → process PCB
```

The PCB does not need to store ALL registers — it just needs to know **where the kernel stack is**. Because all the user registers are already safely sitting on that kernel stack!

---

## Full picture

```
PROCESS A running (user mode)
    user stack active
    kernel stack empty
    PCB: { kstack pointer, state=RUNNING }

timer fires
    hardware pushes user registers → process A kernel stack
    hardware flips to kernel mode

OS runs on process A's kernel stack
    OS decides to switch to process B

    OS saves kernel registers → process A PCB
    //  specifically saves the kernel stack pointer
    //  so it can find all those user registers later

    OS loads process B's kernel stack pointer ← from process B PCB
    OS restores kernel registers from process B PCB

    return-from-trap
    hardware pops user registers ← from process B kernel stack
    hardware flips to user mode

PROCESS B running (user mode)
    has no idea any of this happened
```

---

## One line summary

```
user registers    → saved on process's OWN kernel stack   (by hardware)
kernel stack ptr  → saved in PCB                          (by OS)
```

The PCB is like a **bookmark** to the kernel stack — and the kernel stack holds the actual register values. Two levels of indirection, each serving a purpose.

> **日本語:** "間接" (*kansetsu*) = Indirection — two levels of 間接 is what makes context switching work cleanly! 🎯

> next 

Yes! Exactly right — you just figured out something really important:

---

## The core insight

```
MEMORY (RAM)
┌─────────────────┐
│   code          │ ← never moves, always there
│   globals       │ ← never moves, always there
│   heap          │ ← never moves, always there
│   user stack    │ ← never moves, always there
│   kernel stack  │ ← never moves, always there
└─────────────────┘
```

**None of this is copied anywhere** — it just sits in RAM the whole time, whether the process is running or not.

The ONLY thing that needs saving is:

```
CPU registers  →  kernel stack / PCB
               ↑
               these are the only things that LIVE IN THE CPU CHIP ITSELF
               not in RAM
               so when CPU switches to another process they would be LOST
               unless saved somewhere
```

> **日本語:** "保存" (*hozon*) = Save — you only 保存 what would otherwise be lost

---

## Think of it like this

```
RAM = a desk                  CPU registers = your hands + eyes
───────────────────────────────────────────────────────────────
papers on desk stay there     but what you are currently holding
even when you walk away       and where your eyes are focused
                              THAT needs to be written down
                              before you walk away
```

---

So the entire context switch cost is just:

```
save ~8-16 registers somewhere    (tiny, nanoseconds)
load ~8-16 registers from somewhere

that is it — no memory copying, no stack copying, nothing heavy
```

This is why context switching is **fast** — the OS is not moving gigabytes of process memory around, it is just saving a handful of register values. The rest stays exactly where it is in RAM.

> **日本語:** "効率的" (*kouritsu-teki*) = Efficient — this design is extremely 効率的! 🎯