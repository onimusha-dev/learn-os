# Process

A process is an **abstraction provided by the OS** to describe a running program. When a program sits on disk it is just a file — the moment the OS loads it into memory and starts executing it, it becomes a process.

Every process has its own: memory space, CPU register state, open files, and a unique **PID** (Process ID).

## Process States

A process does not run continuously — the OS moves it between states depending on what is happening:

```
NEW → READY → RUNNING → BLOCKED → READY (again)
                  ↓
             TERMINATED
```

| State | Meaning |
|---|---|
| `NEW` | Process is being created, PCB is being set up |
| `READY` | Process is waiting for the CPU to be assigned to it |
| `RUNNING` | Process is currently being executed on the CPU |
| `BLOCKED` | Process is waiting for something external (I/O, timer, lock) and cannot run even if CPU is free |
| `TERMINATED` | Process has finished, PCB not yet cleaned up |

The OS **scheduler** is what moves processes between READY and RUNNING. A process moves to BLOCKED on its own (by requesting I/O etc.) and moves back to READY when that I/O completes.

---

# Process API

The OS must provide ways to **create, replace, wait for, and delete** processes. In Unix/Linux this is done through **system calls** — your program asking the kernel to do something on its behalf.

In Unix, mainly 3 APIs handle almost everything:

```
fork()  →  exec()  →  wait()
  ↑           ↑          ↑
create      replace    parent
a copy      the copy   waits for
of self     with new   child to
            program    finish
```

## fork()

`fork()` creates an exact clone of the current process. The OS copies the entire PCB, memory, and open files into a new process. After `fork()` two processes exist running the same code — the only difference is what `fork()` returns.

```
main process (pid=100)
      |
   fork()
   /    \
parent  child
pid=100  pid=101
returns  returns
101      0
```

- Returns the **child's PID** to the parent
- Returns **0** to the child
- Returns **-1** if fork failed

This return value difference is how you write code that behaves differently in parent vs child, using a simple if check.

## exec()

`exec()` replaces the calling process's memory and code with a brand new program. The PID stays the same — only the contents change. Nothing after `exec()` runs if it succeeds, because that code no longer exists in memory.

```
Before exec()           After exec()
─────────────           ────────────
pid    = 101    →       pid    = 101   (same, PID never changes)
name   = "child" →      name   = "ls"  (new program)
memory = copy    →      memory = fresh (completely replaced)
state  = RUNNING →      state  = RUNNING
```

The typical pattern is: fork() a child, then exec() inside the child — so the parent survives and the child becomes the new program. This is exactly how your shell works every time you type a command.

## wait()

`wait()` makes the parent block (pause) until a child process finishes. Without it the parent may exit before the child, or the child becomes a **zombie** — a process that has finished but whose PCB is still sitting in memory because nobody collected its exit status.

---

# Signals and Process Termination

The OS communicates with processes using **signals** — small notifications sent to a process or a group of processes.

## Process Groups

When you run a program from the terminal, the OS puts the parent and all its children into the same **process group**. This is how the terminal knows who to notify when you press Ctrl+C.

```
fork()  →  child joins parent's process group
exec()  →  replaces code/memory
           but process group stays the same
           PID stays the same
           signal membership stays the same
```

## Ctrl+C — SIGINT

Pressing Ctrl+C sends **SIGINT** (Signal Interrupt) to the entire process group — not just one process. It does not matter what code the child is running after exec(), it still belongs to the same group and will receive the signal.

```
Ctrl+C
  |
  → SIGINT fires at the whole process group
  → both parent AND child die
  → child dies first (it gets hit directly)
  → parent's wait() unblocks, then parent also dies
```

## Common Signals

| Signal | Trigger | Behavior |
|---|---|---|
| `SIGINT` | Ctrl+C | Polite kill, can be caught or ignored |
| `SIGQUIT` | Ctrl+\ | Kill and produce a core dump |
| `SIGKILL` | kill -9 | Brutal forced kill, cannot be caught or ignored, ever |

## Orphan Processes

If the parent dies but the child is still running, the child becomes an **orphan**. Linux automatically re-parents it to **PID 1** (systemd/init), which will call wait() for it so it is never left uncollected.

```
parent dies
    |
child is still running → becomes orphan
    |
Linux re-parents child to PID 1 (init/systemd)
    |
init adopts it and waits for it to finish
```

## Escaping the Process Group

You can move a child into its own process group before exec() so Ctrl+C no longer reaches it. This is exactly what your terminal does when you run a background job with &.

```cpp
if (pid == 0) {
    setpgrp();  // child creates its own process group
                // Ctrl+C from terminal will no longer reach it
    execvp("sleep", args);
}
```

# Why? Motivating The API
