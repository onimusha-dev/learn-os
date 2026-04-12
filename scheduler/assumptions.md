# **Scheduling — Workload & Metrics**

## Workload Assumptions

Before designing a scheduling policy the OS makes simplified assumptions
about the jobs running in the system — called the **workload**. These are
unrealistic on purpose, and will be relaxed one by one as chapters progress.

1. Each job runs for the same amount of time
2. All jobs arrive at the same time
3. Once started, each job runs to completion
4. All jobs only use the CPU — no I/O
5. The run-time of each job is known

The most unrealistic of these is **#5** — a scheduler that knows every
job's runtime in advance would be omniscient, which is not realistic.

---

## Scheduling Metric — Turnaround Time

To compare scheduling policies we need a metric. For now, one metric:

**Turnaround Time** = time job completes − time job arrived


## Scheduling Metric — Turnaround Time

To compare scheduling policies we need a metric. For now, one metric:

**Turnaround Time** = time job completes − time job arrived

```
Tturnaround = Tcompletion − Tarrival
```

Since all jobs arrive at the same time (assumption #2), Tarrival = 0 for
now, so:

```
Tturnaround = Tcompletion
```

Turnaround time is a **performance** metric — but performance and
**fairness** are often at odds. A scheduler optimizing pure performance
may starve some jobs completely, which is unfair. There is no perfect
solution — every scheduling policy is a tradeoff between the two.
