# **Multi-Level Feedback Queue (MLFQ):**

## The Problem it Solves

Two goals that conflict with each other:
- Minimize **turnaround time** — needs shortest jobs first, but OS doesn't know job lengths
- Minimize **response time** — Round Robin does this but is terrible for turnaround time

MLFQ solves this by **learning job behavior as it runs** instead of needing to know job length upfront.

---

## Basic Structure:

Multiple queues, each with a different priority level:

```cpp
[High Priority]  Q8  →  A  B
                 Q7
                 Q6
                 Q5
                 Q4  →  C
                 Q3
                 Q2
[Low Priority]   Q1  →  D
```

- A job sits in exactly **one queue** at a time
- Higher priority queue = runs first

---

## Basic Rules:

- **Rule 1:** If Priority(A) > Priority(B) → A runs, B doesn't
- **Rule 2:** If Priority(A) = Priority(B) → A and B share CPU in Round Robin

---

## Key Idea:

MLFQ does not give fixed priorities — it **watches job behavior** and adjusts:

- Job keeps giving up CPU early (waiting for keyboard etc.) → **keep priority high** — interactive job
- Job hogs CPU for long periods → **lower its priority** — CPU intensive job

> The scheduler learns from the past to predict the future

## **Attempt #1:** How To Change Priority

### New Rules

- **Rule 3:** New job enters at the **highest priority queue** always
- **Rule 4a:** Job uses its **full time slice** → priority drops one level down
- **Rule 4b:** Job gives up CPU **before** time slice ends → stays at same priority

---

### How it plays out

**Long running job — alone:**
```cpp
enters Q2 → uses full slice → drops to Q1 → uses full slice → drops to Q0 → stays
```
CPU hungry jobs naturally sink to the bottom over time.

**Short interactive job arrives while long job is running:**
```cpp
A (long job) already sitting at Q0
B (short job) arrives → enters Q2 (highest)
B finishes in 2 time slices before ever reaching bottom
A resumes at Q0 after B is done
```
MLFQ assumes every new job *might* be short — gives it high priority first.
If it turns out to be long, it slowly sinks. This is how MLFQ **approximates SJF** without knowing job lengths upfront.

**I/O intensive job:**
```
B does 1ms of CPU work → gives up CPU for I/O → stays at same priority (Rule 4b)
B comes back → still high priority → gets CPU quickly again
```
Interactive jobs that frequently wait for I/O naturally **stay at the top** because they never exhaust their time slice.

---

### The core insight

```
new job → assume short → give high priority
    |
    stays short?  → finishes fast at top         ✅ interactive
    turns out long? → slowly sinks to bottom     ✅ batch/CPU bound
```

## Problems With Attempt #1

### Problem 1 — Starvation
Too many interactive jobs at the top → they consume all CPU time →
long running jobs at the bottom **never get to run**.

```cpp
Q2  → interactive job, interactive job, interactive job...
Q0  → long job sitting here, never gets CPU ☠️
```

### Problem 2 — Gaming the Scheduler
A sneaky program can **abuse Rule 4b** — give up CPU just before the
time slice ends (fake I/O), so it never gets demoted:

```cpp
time slice = 10ms
sneaky job runs for 9.99ms → fires useless I/O → gives up CPU
→ stays at same high priority queue
→ repeat forever
→ monopolizes CPU while playing by the "rules"
```

### Problem 3 — Job Behavior Changes Over Time
A job that started as CPU-bound may later become interactive — but it
is already stuck at the bottom queue with no way back up:

```cpp
job starts → CPU hungry → sinks to Q0
job later needs fast response (becomes interactive)
→ still stuck at Q0, treated as batch job forever
```

---

### Summary

| Problem | Cause |
|---|---|
| Starvation | too many high priority jobs block low priority ones |
| Gaming | Rule 4b can be exploited with fake I/O |
| No recovery | a job's behavior can change but priority cannot go back up |

## Attempt #2 — Priority Boost

### New Rule

- **Rule 5:** After every time period **S**, move ALL jobs to the topmost queue

```c
every S milliseconds:
    every job in the system → bumped to Q2 (top)
```

---

### Problems it fixes

**Starvation** — long running job stuck at bottom now gets boosted up
every S ms, guaranteed to get some CPU time eventually.

**Behavior change** — a job that was CPU-bound but became interactive
gets a fresh start at the top instead of being stuck at the bottom forever.

---

### The tradeoff — what should S be?

```c
S too high  →  long running jobs starve between boosts
S too low   →  interactive jobs get interrupted too often, lose their fair share
```

There is no perfect value for S — it depends entirely on the workload.
OSTEP calls these kinds of values **voodoo constants** — values that
require educated guessing rather than a clean formula to set correctly.

---

## Attempt #3 — Better Accounting

### The Fix

Rules 4a and 4b are replaced with a single rule:

- **Rule 4:** Once a job uses up its **total time allotment** at a given level
(regardless of how many times it gave up the CPU), its priority drops.

---

### What changed

Before — the scheduler forgot CPU usage every time a job gave up the CPU:
```c
time slice = 10ms
job runs 9ms → fake I/O → timer resets → job runs 9ms → fake I/O → never demoted
```

After — the scheduler **accumulates** CPU usage across all visits to that queue:
```c
time slice = 10ms
job runs 9ms → fake I/O → comes back → runs 1ms → total = 10ms → demoted ✅
```

It does not matter how the job uses its allotment — one long burst or many
small ones — once the total is used up at that level, it moves down.

---

### All 5 rules together now

| Rule | Description |
|---|---|
| Rule 1 | Higher priority runs first |
| Rule 2 | Same priority → Round Robin |
| Rule 3 | New job enters at highest queue |
| Rule 4 | Job uses full allotment at a level → demoted (regardless of I/O tricks) |
| Rule 5 | Every S ms → all jobs boosted to top queue |

