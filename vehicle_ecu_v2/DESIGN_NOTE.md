# Vehicle ECU Simulation — Design Note
**Version:** 1.3.0 | **Language:** C11 | **Standard:** MISRA-C:2012

---

## 1. Scheduler Flow

The scheduler runs one fixed-order cycle per tick. The order is mandatory and never changes:

```
1. read_inputs()           – Load sensor values (scenario or interactive)
2. validate_inputs()       – Range-check and sanitize all inputs
3. update_mode()           – Apply mode transitions; detect illegal ones
4. run_control_checks()    – Check temp, gear, overspeed, speed-in-ACC/OFF
5. update_fault_status()   – Track persistence counters and primary fault
6. evaluate_system_state() – Decide NORMAL / DEGRADED / SAFE
7. log_cycle_summary()     – Print console output + write ecu_last_cycle.log
```

Each step depends on the output of the previous step. Control checks must run after the mode is finalized (step 3) so they know whether the engine is on. Fault status must be updated before the state is evaluated so persistence is counted correctly.

---

## 2. Module Responsibilities

| Module | File | Responsibility |
|---|---|---|
| Input | input.c / input.h | Read sensor values; validate ranges; handle scenario vs interactive mode |
| Mode | mode.c / mode.h | Enforce legal mode transitions; latch FAULT on illegal ones |
| Control | control.c / control.h | Check each sensor fault condition; set or clear faults |
| Fault | fault.c / fault.h | Maintain fault bit flags, counters, persistence, and primary fault |
| State | state.c / state.h | Map active faults to system state (NORMAL / DEGRADED / SAFE) |
| Log | log.c / log.h | Console output (color-coded) and plain-text file output |
| Types | types.h | All shared enums, structs, and constants — no logic |
| Main | main.c | Scheduler loop, test scenarios, interactive mode, reset logic |

---

## 3. State Transition Logic

```
NORMAL ──[any fault active]──────────────► DEGRADED
DEGRADED ──[fault cleared]───────────────► NORMAL
DEGRADED ──[persistent fault ≥ threshold]► SAFE  (latched)
ANY ──[FAULT_INVALID_GEAR detected]──────► SAFE  (latched, immediate)
ANY ──[MODE_FAULT entered]───────────────► SAFE  (latched, immediate)
SAFE ──[state_reset() called]────────────► NORMAL
```

Key rules:
- SAFE is a **latch state** — once entered, only `state_reset()` can exit it.
- FAULT_INVALID_GEAR immediately latches SAFE, not via the persistence path.
- Persistence threshold: 3 consecutive cycles of the same fault → fault marked persistent; 1 persistent fault → SAFE.

---

## 4. Fault Handling Strategy

**Fault priority** is determined by enum index (lower = higher priority). When multiple faults fire in one cycle, the one with the lowest `FaultType_t` index is the PRIMARY FAULT shown in the summary.

```
Priority 1 (highest): FAULT_CRITICAL_OVERHEAT  – temp > 110°C
Priority 2:           FAULT_INVALID_GEAR        – gear > 5 in any mode
Priority 3:           FAULT_OVERSPEED           – speed > 120 kph
Priority 4:           FAULT_INVALID_MODE        – illegal mode transition
Priority 5:           FAULT_SPEED_IN_OFF        – speed > 0 in OFF mode
Priority 6 (lowest):  FAULT_SPEED_IN_ACC        – speed while in ACC mode
```

**Persistence tracking:** Each cycle a fault stays active, its `persistent_counts[]` counter increments. If it reaches `FAULT_PERSISTENT_THRESHOLD` (3), it is marked persistent. A persistent fault count ≥ `FAULT_SAFE_STATE_THRESHOLD` (1) immediately forces STATE_SAFE.

**Fault latch:** FAULT_INVALID_GEAR and MODE_FAULT set `s_safe_latch = true` directly, bypassing the persistence path. This ensures immediate SAFE on these conditions regardless of how many cycles they have been active.

**Reset:** `fault_reset_all()` called from `state_reset()` clears all flags, counters, and persistence state.
