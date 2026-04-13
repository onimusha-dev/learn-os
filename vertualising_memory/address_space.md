# Memory Virtualization — Address Spaces

## How it evolved

**Early systems** — one OS, one program, both just sitting in physical memory
directly. Simple, but useless for multitasking.

**Multiprogramming** — multiple processes loaded in memory at once, OS
switches between them. Problem: saving entire memory to disk on every
switch is brutally slow.

**Solution** — leave all processes in memory at the same time, just switch
the CPU between them. Now protection becomes critical — processes must
not touch each other's memory.

---

## The Address Space

Every process gets an **address space** — the OS's illusion of private memory.
Three regions inside it:

```c
0KB   [ code  ]  ← static, loaded once, never moves
1KB   [ heap  ]  ← grows downward, malloc/new lives here
      [  ...  ]
16KB  [ stack ]  ← grows upward, local variables, function calls
```

Heap and stack are placed at opposite ends so they can both grow
without immediately colliding.

> Every pointer, every address you ever print in a program is a
> **virtual address** — not the real physical location in RAM. Only
> the OS and hardware know the real address.

---

## Virtual Memory Goals

| Goal | Meaning |
|---|---|
| **Transparency** | process thinks it owns all memory, has no idea virtualization is happening |
| **Efficiency** | translation must be fast, hardware (TLB) helps with this |
| **Protection** | processes cannot read or write each other's memory, ever |

---

## The Core Idea

```c
process A thinks it lives at address 0
reality: it is loaded at physical address 320KB

process A reads address 0
    → OS + hardware translates → physical address 320KB
    → process A never knows the difference
```

This translation happening silently on every memory access is the
entire foundation of modern OS memory management.


# **Memory API — Stack & Heap**

## Two Types of Memory

**Stack** — managed automatically by the compiler:
```c
void func() {
    int x;  // compiler allocates this on entry, frees on return
}
```
Lives only as long as the function is running. Gone when function returns.

**Heap** — managed manually by you:
```c
void func() {
    int *x = (int *) malloc(sizeof(int));  // you allocate
    // you must free it yourself later
}
```
Lives until you explicitly free it — survives beyond the function call.

---

## malloc()

```c
#include <stdlib.h>
void *malloc(size_t size);
```

- Pass how many bytes you need
- Returns a pointer to the allocated memory
- Returns `NULL` if it fails — always check for NULL!
- Returns `void *` — a raw address, you cast it to the type you need

---

## sizeof() — always use it instead of raw numbers

```c
// ✅ correct
double *d = (double *) malloc(sizeof(double));

// ❌ bad practice
double *d = (double *) malloc(8);
```

**Tricky case — pointer vs array:**
```c
int *x = malloc(10 * sizeof(int));
printf("%d\n", sizeof(x));   // prints 4 or 8 — size of the POINTER not the array!

int x[10];
printf("%d\n", sizeof(x));   // prints 40 — compiler knows the full array size
```

`sizeof()` on a pointer only tells you the size of the pointer itself,
not how much heap memory it points to.

**Strings — never use sizeof, use strlen + 1:**
```c
// ✅ correct — +1 for the null terminator \0
char *s = malloc(strlen(str) + 1);

// ❌ wrong — sizeof gives pointer size, not string length
char *s = malloc(sizeof(str));
```

---

## The cast on malloc

```c
int *x = (int *) malloc(sizeof(int));
//        ^^^^^^
//        cast — not required in C, just tells the compiler and
//        other developers "I know what type this is"
```

malloc returns `void *` — casting it is just good communication,
not a functional requirement.

---

## Key Rules

| Rule | Reason |
|---|---|
| Always use `sizeof()` | never hardcode byte sizes |
| Always check for `NULL` | malloc can fail if memory is full |
| Strings need `strlen + 1` | null terminator needs one extra byte |
| Heap memory must be freed | unlike stack, it does not clean itself up |

# Memory API — free() & Common Errors

## free()

```c
int *x = malloc(10 * sizeof(int));
// ... use x ...
free(x);  // pass the same pointer back, that's it
```

You do not pass the size — the memory library tracks that itself.
`malloc()` and `free()` are **library calls**, not system calls — they
manage space inside your virtual address space.

---

## Common Errors

**1. Forgetting to allocate:**
```c
char *dst;            // just a pointer, no memory behind it
strcpy(dst, src);     // segfault ☠️

// ✅ fix
char *dst = malloc(strlen(src) + 1);
strcpy(dst, src);
```

**2. Not allocating enough — buffer overflow:**
```c
char *dst = malloc(strlen(src));    // missing +1 for null terminator
strcpy(dst, src);                   // writes one byte past the end
```
May seem to work sometimes — but it is a time bomb and a common
source of security vulnerabilities.

**3. Forgetting to initialize allocated memory:**
```c
int *x = malloc(sizeof(int));
printf("%d\n", *x);   // reading garbage — unknown value in heap
```
Use `calloc()` instead if you need memory zeroed out automatically.

**4. Memory leak — forgetting to free:**
```c
int *x = malloc(sizeof(int));
// ... use x ...
// forgot free(x) — memory sits there forever
```
In short programs the OS cleans it up on exit. In long-running servers
or the OS itself this slowly eats all available memory until crash.

**5. Dangling pointer — freeing too early:**
```c
free(x);
printf("%d\n", *x);   // x is freed but you are still using it ☠️
```

**6. Double free:**
```c
free(x);
free(x);   // undefined behavior, likely crash ☠️
```

**7. Invalid free:**
```c
int x = 5;
free(&x);   // x is on the stack, not heap — never pass this to free ☠️
```

---

## Other useful calls

| Call | Purpose |
|---|---|
| `calloc(n, size)` | allocates AND zeroes memory — safer than malloc |
| `realloc(ptr, newsize)` | grows an existing allocation, copies old data over |

---

## Under the Hood

`malloc` and `free` are built on top of OS system calls:

```c
malloc/free          ← library, manages your heap
    ↓
brk() / sbrk()       ← system calls, move the heap boundary
mmap()               ← system call, get fresh memory pages from OS
```

Never call `brk()` or `sbrk()` directly — that is malloc's job.

---

## Two Levels of Memory Management

```
OS level    → gives memory pages to processes, takes them back on exit
Process level → malloc/free manages the heap inside those pages
```

Even if you leak memory, the OS reclaims everything when the process
dies. But in long-running programs — servers, databases, the OS itself —
leaks are fatal over time.

