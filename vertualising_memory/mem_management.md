# Free Space Management

## The Problem

When memory is divided into **variable-sized chunks**, free space gets
chopped into scattered pieces — external fragmentation:

```c
[ free 10KB ][ used 10KB ][ free 10KB ]

request for 15KB → FAILS
total free = 20KB but no single contiguous chunk is 15KB ❌
```

---

## Core Mechanisms

### Splitting
When a request is smaller than a free chunk, split the chunk in two —
give the first part to the caller, keep the rest on the free list:

```c
free list before:  [ addr:0 len:10 ] → [ addr:20 len:10 ]
request for 1 byte from chunk at addr:20

free list after:   [ addr:0 len:10 ] → [ addr:21 len:9 ]
                                          ↑ shrunk by 1
```

### Coalescing
When memory is freed, check if neighboring chunks are also free —
if yes, merge them into one larger chunk:

```c
free then free then free → without coalescing:
[ addr:0 len:10 ] → [ addr:10 len:10 ] → [ addr:20 len:10 ]
request for 20KB → FAILS even though all memory is free ❌

with coalescing:
[ addr:0 len:30 ] ✅
```

Always coalesce on `free()` — otherwise the heap looks fragmented
even when it is completely empty.

---

## Tracking Allocation Size — The Header

`free(ptr)` takes no size argument — so how does the allocator know
how big the chunk is? It secretly stores a **header** just before every
allocated chunk:

```c
typedef struct {
    int size;   // how big is this allocation
    int magic;  // sanity check value e.g. 1234567
} header_t;
```

```c
Memory layout after malloc(20):

[ header: size=20, magic=1234567 ][ 20 bytes given to user ]
 ↑                                 ↑
 hptr                              ptr  ← what malloc() returns
```

When `free(ptr)` is called:
```c
header_t *hptr = ptr - sizeof(header_t);  // step back to find header
assert(hptr->magic == 1234567);            // sanity check
// now hptr->size tells us how big to free
```

Important: when you request N bytes, allocator actually needs
**N + `sizeof(header)`** bytes of free space.

---

## The Free List Lives Inside the Heap

The free list is not stored separately — it is **embedded inside the free
space itself** using nodes:

```c
typedef struct node_t {
    int size;
    struct node_t *next;
} node_t;
```

```c
Initial 4KB heap:
[ node: size=4088, next=NULL ][ ...rest of 4KB heap... ]

after malloc(100):
[ header: size=100 ][ 100 bytes ][ node: size=3980, next=NULL ]

after free() of middle chunk:
[ node: size=100, next=→ ]──────────────────→[ node: size=3764 ]
```

---

## Allocation Strategies

### Best Fit
Search entire free list → return smallest chunk that fits the request.
Minimizes wasted space but requires full list traversal every time.

### Worst Fit
Search entire list → return the **largest** chunk.
Tries to leave big chunks available but performs badly in practice —
still requires full traversal and causes excess fragmentation.

### First Fit
Return the **first** chunk that is big enough. Fast — no full traversal.
Can pollute the start of the list with small leftover chunks over time.

### Next Fit
Like first fit but remembers where the last search ended and starts
from there next time. Spreads fragmentation more evenly across the list.

**Example with free list [ 10 | 30 | 20 ], request for 15:**

```c
Best fit  → uses 20 → leaves [ 10 | 30 | 5  ]  (smallest leftover)
Worst fit → uses 30 → leaves [ 10 | 15 | 20 ]  (largest leftover)
First fit → uses 30 → leaves [ 10 | 15 | 20 ]  (same as worst fit here)
```

---

## Advanced Approaches

### Segregated Lists
Keep a **separate free list per common object size**. When a request
comes in for that exact size — serve it instantly from the dedicated list,
no search needed. Used in the **slab allocator** (Linux kernel) for
frequently allocated kernel objects like locks and inodes.

### Buddy Allocation
Free space is always split into **powers of 2**. To find space for a
request, recursively halve until a fitting block is found:

```c
64KB → 32KB | 32KB
             → 16KB | 16KB
                      → 8KB | 8KB   ← allocate left 8KB for 7KB request
```

When freed, check if the **buddy** (the other half of the split) is also
free — if yes, merge back up recursively. Finding the buddy is cheap:
buddy addresses only differ by **one bit**.
Downside: internal fragmentation since you can only give out
power-of-2 sized blocks.

---

## Growing the Heap

When the heap runs out of space the allocator asks the OS for more
memory via `sbrk()` system call — OS maps new physical pages into
the process address space and extends the heap boundary.

---

## Summary

| Mechanism | Purpose |
|---|---|
| Splitting | serve requests smaller than a free chunk |
| Coalescing | merge neighboring free chunks on free() |
| Header | track allocation size without user passing it |
| Embedded free list | manage free space inside the free space itself |
| Best/Worst/First/Next fit | different tradeoffs of speed vs fragmentation |
| Segregated lists | fast fixed-size allocation, used in kernel slab allocator |
| Buddy allocation | power-of-2 splits, easy coalescing via buddy bit |
