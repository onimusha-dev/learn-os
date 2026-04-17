# Segmentation

## The Problem with Base and Bounds

Base and bounds loads the **entire address space** into physical memory
including the empty space between heap and stack — pure waste:

```cpp
0KB   [ code  ]  2KB used
2KB   [ heap  ]  2KB used
4KB   [       ]
...   [ free  ]  ← this entire gap sits in physical RAM doing nothing
...   [       ]
14KB  [ stack ]  2KB used
16KB
```

In a 32-bit address space (4GB) a program using only a few MB would
still demand 4GB of physical memory. Not acceptable.

---

## Segmentation — One Base/Bounds Per Segment

Instead of one base/bounds pair for the whole address space, give
**each segment its own base and bounds pair** in the MMU:

```c
Segment    Base     Size
────────────────────────
Code       32KB     2KB
Heap       34KB     2KB
Stack      28KB     2KB
```

Each segment is placed **independently** anywhere in physical memory.
The empty space between heap and stack never touches physical RAM at all.

```c
Physical Memory:
0KB   [ OS          ]
16KB  [ (free)      ]
28KB  [ Stack       ]
30KB  [ (free)      ]
32KB  [ Code        ]
34KB  [ Heap        ]
36KB  [ (free)      ]
64KB
```

---

## Address Translation — The Offset

This is the key part. You cannot just add the virtual address directly
to the segment base — you need to find the **offset within that segment**
first.

**Offset = virtual address − segment's starting virtual address**

Then:
**physical address = segment base + offset**

---

### Example 1 — Code segment (easy case)

Code starts at virtual address 0. So offset = virtual address directly.

```c
virtual address = 100
code starts at virtual 0 → offset = 100 - 0 = 100
code base in physical RAM = 32KB = 32768

physical address = 32768 + 100 = 32868 ✅
bounds check: 100 < 2KB ✅
```

---

### Example 2 — Heap segment (the tricky one)

Heap starts at virtual address 4KB (4096), NOT at 0.
So you cannot just use the raw virtual address — that would overshoot.

```c
virtual address = 4200
heap starts at virtual 4096 → offset = 4200 - 4096 = 104
heap base in physical RAM = 34KB = 34816

physical address = 34816 + 104 = 34920 ✅
bounds check: 104 < 2KB ✅
```

If you had just done 34816 + 4200 = 39016 — completely wrong physical
address, pointing into some other segment's memory.

---

### Example 3 — Out of bounds

```c
virtual address = 7KB (7168)
heap starts at virtual 4096 → offset = 7168 - 4096 = 3072
bounds check: 3072 > 2KB ❌ → SEGMENTATION FAULT ☠️
```

This is where the famous **segfault** comes from — accessing a virtual
address that falls outside the bounds of its segment. The term stuck
even in modern systems that do not use segmentation anymore.

---

## How hardware knows which segment an address belongs to

The top bits of the virtual address are used as a **segment selector**:

```c
virtual address in binary:
[ segment bits ] [ offset bits ]
      ↑                ↑
  which segment    how far into
  (code/heap/stack)  that segment
```

Hardware reads segment bits → picks the right base/bounds pair →
adds offset → checks bounds → done.

---

## Summary

| Concept | Meaning |
|---|---|
| Segment | contiguous region of address space (code, heap, stack) |
| Offset | distance from the start of that segment |
| Segfault | accessing an address outside a segment's bounds |
| Advantage over base/bounds | no wasted physical RAM for unused address space |
| Each segment | gets its own independent base and bounds in the MMU |



# Segmentation — Full Details

## How Hardware Knows Which Segment

The **top bits** of the virtual address select the segment:

```c
14-bit virtual address:
[ 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 ]
[  seg  ]  [            offset (12 bits)                      ]

top 2 bits:
00 → code segment
01 → heap segment
11 → stack segment
```

**Example — virtual address 4200 (heap):**
```c
4200 in binary = 01 000001101000
                 ^^
                 01 → heap segment
                    ^^^^^^^^^^^^
                    offset = 0000 0110 1000 = 104

physical address = heap base (34KB) + 104 = 34920 ✅
```

**Hardware pseudocode:**
```c
segment = (VirtualAddress & SEG_MASK) >> SEG_SHIFT  // top 2 bits
offset  = VirtualAddress & OFFSET_MASK              // bottom 12 bits

if (offset >= Bounds[segment])
    RaiseException(PROTECTION_FAULT)                // segfault ☠️
else
    PhysAddr = Base[segment] + offset               // translation ✅
```

---

## The Stack — Grows Backwards

Stack is special — it grows in the **negative direction**. Hardware needs
an extra bit per segment to know which way it grows:

```c
Segment    Base    Size    Grows Positive?
──────────────────────────────────────────
Code       32KB    2KB     1  (positive)
Heap       34KB    2KB     1  (positive)
Stack      28KB    2KB     0  (negative ← special)
```

**Stack translation example — virtual address 15KB:**
```c
15KB in binary = 11 1100 0000 0000
                 ^^
                 11 → stack segment
                    offset = 3KB

max segment size = 4KB
negative offset  = 3KB - 4KB = -1KB

physical address = base (28KB) + (-1KB) = 27KB ✅
bounds check: absolute value of -1KB (1KB) < size (2KB) ✅
```

The key insight — for a negative growing segment you subtract the
max segment size from the offset to get the negative offset, then add
to base.

---

## Sharing & Protection Bits

Segments can be **shared between processes** — useful for code since
it never changes. Each segment gets protection bits:

```c
Segment    Base    Size    Grows Positive?    Protection
────────────────────────────────────────────────────────
Code       32KB    2KB     1                  Read-Execute
Heap       34KB    2KB     1                  Read-Write
Stack      28KB    2KB     0                  Read-Write
```

Multiple processes can map their code segment to the **same physical
memory** — OS shares it secretly, processes never know. If a process
tries to write to a read-only segment → hardware raises exception → OS
terminates the process.

---

## OS Issues with Segmentation

**Context switch:**
All segment registers (base, bounds, grows-positive, protection) must
be saved to the PCB and restored on every context switch — just like
the regular base/bounds pair before.

**External Fragmentation:**
Because segments are variable sized, free memory gets chopped into
random sized holes:

```c
Physical Memory:
[ OS ]		[ free 8KB ]
[ segment ] [ free 4KB ]
[ segment ] [ free 12KB ]

request comes in for 20KB
total free = 24KB but no single contiguous chunk is 20KB ❌
```

This is **external fragmentation** — enough total free memory exists
but it is scattered in unusable pieces.

**Solutions:**

| Solution | Problem |
|---|---|
| **Compaction** — stop processes, copy segments together, update registers | extremely expensive, CPU intensive |
| **Best-fit** — find free chunk closest in size to request | still leaves small leftover holes |
| **Worst-fit** — use largest free chunk | leaves large remainders but wastes big chunks |
| **First-fit** — use first chunk that fits | fast but fragments the beginning of memory |

No algorithm fully solves external fragmentation — it is a fundamental
problem with variable-sized allocation.

---

## Segmentation vs Base and Bounds

| | Base & Bounds | Segmentation |
|---|---|---|
| Base/bounds pairs | 1 per process | 1 per segment (3+) |
| Unused space in RAM | yes — whole gap allocated | no — each segment placed independently |
| External fragmentation | no | yes |
| Code sharing | no | yes (via protection bits) |
| Stack direction support | no | yes (grows-positive bit) |

---

## Why Segmentation is Still Not Enough

Even with segmentation, the heap must be **entirely in memory** even
if only a small part of it is being used. A large sparse heap wastes
physical memory just like before — just at a segment level instead of
address space level.

The real solution is to never allocate variable sized chunks at all —
which leads directly to the next technique: **Paging**.