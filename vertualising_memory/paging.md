# Paging — Introduction

## The Core Idea

Segmentation used variable-sized chunks → external fragmentation.
Paging fixes this by using **fixed-sized chunks** called **pages**.

```c
Virtual address space  →  divided into fixed-size PAGES;
Physical memory        →  divided into fixed-size FRAMES (same size)
```

Any page can go into any free frame — no contiguous requirement,
no fragmentation.

---

## Simple Example

```c
64-byte address space, 16-byte pages → 4 virtual pages
128-byte physical memory             → 8 physical frames

Virtual pages placed into physical frames:
VP 0 → PF 3
VP 1 → PF 7
VP 2 → PF 5
VP 3 → PF 2

Frames 1, 4, 6 → free
```

The OS just grabs any 4 free frames from its free list — no alignment,
no contiguous requirement, no fragmentation at all.

---

## Page Table

The OS keeps a **per-process page table** that maps every virtual page
to its physical frame:

```c
Page Table (per process):
VP 0  →  PF 3
VP 1  →  PF 7
VP 2  →  PF 5
VP 3  →  PF 2
```

Every process has its own page table. Stored in kernel memory.
On context switch the OS switches to the new process's page table.

---

## Address Translation — VPN + Offset

A virtual address is split into two parts:

```c
virtual address = [ VPN | offset ]
                     ↑       ↑
              which page   which byte
                           within that page
```

**How many bits for each?**
```c
address space = 64 bytes  → need 6 bits total  (2^6 = 64)
page size     = 16 bytes  → need 4 bits offset (2^4 = 16)
num pages     = 4         → need 2 bits VPN    (2^2 = 4)

virtual address layout:
[ Va5 Va4 | Va3 Va2 Va1 Va0 ]
[   VPN   |     offset      ]
```

**Translation steps:**
```
1. split virtual address into VPN + offset
2. use VPN to index into page table → get PFN (physical frame number)
3. physical address = PFN + offset (offset never changes!)
```

**Example — virtual address 21:**
```c
21 in binary = 010101
               ↓↓ ↓↓↓↓
               01 0101
              VPN offset
               1    5

page table: VP1 → PF7
PF7 in binary = 111

physical address = 111 0101 = 117 ✅
```

The offset (which byte within the page) stays identical —
only the VPN gets replaced with the PFN.

---

## Why Paging Wins Over Segmentation

| Problem | Segmentation | Paging |
|---|---|---|
| External fragmentation | yes — variable sizes | no — all frames same size |
| Free space management | complex, needs coalescing | simple, just a list of free frames |
| Stack/heap direction | must track grow direction | does not matter |
| Sparse address spaces | still wastes RAM | each page mapped independently |

---

## What Comes Next

Paging raises new questions:

- Where exactly is the page table stored in memory?
- Page tables can be huge — how do we make them smaller?
- Translation on every memory access is slow — how do we speed it up?

Answers: **TLBs** (speed) and **multi-level page tables** (size).