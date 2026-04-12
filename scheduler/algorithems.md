# Scheduling — Introduction

## The Goal

The OS needs a **policy** to decide which process runs next on the CPU.
Different policies make different tradeoffs — there is no perfect answer.

---

## Workload Assumptions (simplified to start)

1. Each job runs for the same amount of time
2. All jobs arrive at the same time
3. Once started, each job runs to completion
4. All jobs only use the CPU — no I/O
5. The run-time of each job is known

These are relaxed one by one as each algorithm improves.

---

## Metrics

**Turnaround Time** — how long from arrival to completion:
```py
Tturnaround = Tcompletion − Tarrival
```

**Response Time** — how long from arrival to first time on CPU:
```py
Tresponse = Tfirstrun − Tarrival
```

Turnaround and response time are often at odds with each other —
optimizing one tends to hurt the other.

**Fairness** — every job gets a reasonable share of the CPU. Fairness
and performance also conflict — an unfair scheduler can be faster but
starves some jobs.

---

## FIFO — First In First Out

Run jobs in the order they arrive, each to completion.

```c
A(10ms) → B(10ms) → C(10ms)
avg turnaround = (10 + 20 + 30) / 3 = 20ms   ✅
```

**Problem — Convoy Effect:**
If a long job arrives first, short jobs pile up behind it waiting:
```c
A(100ms) → B(10ms) → C(10ms)
avg turnaround = (100 + 110 + 120) / 3 = 110ms   ❌
```

---

## SJF — Shortest Job First

Always run the shortest job first.

```c
B(10ms) → C(10ms) → A(100ms)
avg turnaround = (10 + 20 + 120) / 3 = 50ms   ✅
```

**Problem — late arrivals:**
If A arrives at t=0 and B,C arrive at t=10, SJF still runs A to
completion first since it already started. Short jobs still wait.

```c
A starts at t=0, B and C arrive at t=10 but must wait for A
avg turnaround = 103ms   ❌
```

---

## STCF — Shortest Time to Completion First

SJF with **preemption** — when a new job arrives, if it has less time
remaining than the current job, preempt and run the shorter one.

```c
A starts at t=0
B and C arrive at t=10 → preempt A, run B and C first
avg turnaround = (120-0 + 20-10 + 30-10) / 3 = 50ms   ✅
```

Great for turnaround — but terrible for response time. The third job
still has to wait for the first two to fully complete before it ever runs.

---

## Round Robin (RR)

Instead of running jobs to completion, run each job for a small fixed
**time slice** then switch to the next job. Cycles through all jobs repeatedly.

```c
time slice = 1ms
A B C A B C A B C ...   (interleaved)
avg response time = (0 + 1 + 2) / 3 = 1ms   ✅
```

**Tradeoff — time slice length:**

```c
too short  →  context switching overhead dominates
too long   →  starts to feel like FIFO, response time suffers
```

RR is great for response time but terrible for turnaround — it stretches
every job out as long as possible by never letting any one finish quickly.

---

## Incorporating I/O

When a job does I/O it is **blocked** — not using the CPU at all.
Wasting that CPU time is inefficient.

**Solution — treat each CPU burst as a separate job:**

```c
A does: 10ms CPU → 10ms I/O → 10ms CPU → 10ms I/O ...
B does: 50ms CPU (no I/O)

Poor approach:        A A A A A B B B B B   (no overlap)
Better approach:      A B A B A B A B A B   (overlap I/O with B's CPU work)
```

While A waits for I/O, run B on the CPU. Overlap keeps the CPU busy
and dramatically improves utilization.

---

## Summary — The Fundamental Tradeoff

| Algorithm | Good at | Bad at |
|---|---|---|
| FIFO | simple | convoy effect |
| SJF | turnaround | late arrivals, response time |
| STCF | turnaround | response time |
| RR | response time | turnaround time |

No algorithm wins on all metrics — every scheduling policy is a
tradeoff. The next step is MLFQ, which learns job behavior at runtime
to get the best of both worlds without knowing job lengths upfront.