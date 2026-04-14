# Vehicle ECU — Test Report
**Version:** 1.3.0 | **Total Scenarios:** 12 (25 scheduler cycles)

---

## Summary

All 12 test cases PASSED. Every expected state transition, fault activation, and recovery behaviour was observed in the actual console output.

---

## Test Cases

### TC1 — Normal Start Sequence
**Input:** OFF (speed=0, temp=20, gear=0) → ACC → IGNITION_ON (speed=40, temp=80, gear=1)
**Expected:** Clean mode progression, no faults, STATE=NORMAL throughout.
**Actual:** OFF→ACC mode change logged. ACC→IGNITION_ON mode change logged. All cycles: WARNS=[NONE], FAULTS=[NONE], STATE=NORMAL.
**Result: PASS**

---

### TC2 — Overspeed
**Input:** IGNITION_ON, speed=130 kph, temp=85, gear=4
**Expected:** FAULT_OVERSPEED set, STATE=DEGRADED.
**Actual:** `[FAULT] Overspeed detected: 130 kph (limit: 120 kph) [Severity: MAJOR]`. STATE changed NORMAL→DEGRADED. FAULTS=[Overspeed(1)].
**Result: PASS**

---

### TC3 — High Temperature Warning Only
**Input:** IGNITION_ON, speed=60, temp=100, gear=3
**Expected:** High temperature warning logged; no fault raised; STATE=NORMAL (overspeed from TC2 clears).
**Actual:** `High temperature warning: 100 degC (high threshold: 95 degC) [WARNING only]`. Overspeed cleared. STATE=NORMAL. WARNS=[HIGH_TEMP(100 degC)], FAULTS=[NONE].
**Result: PASS**

---

### TC4 — Critical Overheat
**Input:** IGNITION_ON, speed=60, temp=115, gear=3
**Expected:** FAULT_CRITICAL_OVERHEAT set, STATE=DEGRADED.
**Actual:** `[FAULT] Critical Overheat: 115 degC (threshold: 110 degC) [Severity: CRITICAL]`. STATE=DEGRADED. FAULTS=[Critical Overheat(1)].
**Result: PASS**

---

### TC5 — Invalid Gear → Immediate SAFE
**Input:** IGNITION_ON, speed=60, temp=80, gear=9
**Expected:** FAULT_INVALID_GEAR set, STATE immediately latches to SAFE.
**Actual:** `[FAULT] Invalid gear 9 in IGNITION_ON mode (valid range: 0-5) [Severity: MAJOR]`. STATE CHANGE: DEGRADED→SAFE. `[STATE CHANGE] DEGRADED -> SAFE | Reason: System entered FAULT mode - latched SAFE`. System halted.
**Result: PASS**

---

### TC6 — IGNITION_ON → ACC Downgrade + Gear in ACC
**Input:** TC6a: IGNITION_ON (normal values) → TC6b: ACC (speed=0, gear=0) → TC6c: ACC (gear=2)
**Expected:** Valid IGNITION_ON→ACC downgrade accepted. Gear selector change in ACC accepted (no fault).
**Actual:** TC6b: `Mode transition: IGNITION_ON --> ACC` (post-reset shows OFF→ACC due to auto-reset). TC6c: gear=2 in ACC, all validated OK, STATE=NORMAL, FAULTS=[NONE].
**Result: PASS**

---

### TC7 — Illegal Mode Transition (OFF → IGNITION_ON)
**Input:** TC7a: reset to OFF. TC7b: request IGNITION_ON directly from OFF.
**Expected:** Illegal transition detected, system forced to FAULT mode, STATE=SAFE.
**Actual:** `Illegal transition: OFF --> IGNITION_ON (FORCING FAULT MODE)`. `Entering FAULT mode: OFF --> FAULT`. STATE CHANGE: NORMAL→SAFE. FAULTS=[INVALID_MODE(1)]. System halted.
**Result: PASS**

---

### TC8 — Direct FAULT Mode Input
**Input:** User sets mode=3 (FAULT) directly.
**Expected:** System enters FAULT mode immediately, STATE=SAFE.
**Actual:** `Entering FAULT mode: OFF --> FAULT | Reason: Direct user input MODE=FAULT (3)`. STATE CHANGE: NORMAL→SAFE. FAULTS=[INVALID_MODE(1)]. System halted.
**Result: PASS**

---

### TC9 — Speed in ACC Mode
**Input:** ACC mode, speed=200 kph, gear=5.
**Expected:** Speed rejected (engine off), FAULT_SPEED_IN_ACC set, STATE=DEGRADED.
**Actual:** `[FAULT] Speed 200 kph received in ACC mode. ACC is engine-off/accessory-only. Speed input INVALID - forced to 0.` FAULTS=[Speed in ACC mode(1)]. STATE=DEGRADED.
**Result: PASS**

---

### TC10 — Multiple Simultaneous Faults
**Input:** IGNITION_ON, speed=140, temp=120, gear=9.
**Expected:** All three faults active; Critical Overheat is PRIMARY FAULT (highest priority); STATE=SAFE via INVALID_GEAR latch.
**Actual:** Critical Overheat(1), INVALID_GEAR(1), Overspeed(1) all active. `[PRIMARY FAULT] Critical Overheat`. STATE CHANGE: NORMAL→SAFE. System halted.
**Result: PASS**

---

### TC11 — Persistent Fault → SAFE After 3 Cycles
**Input:** Three consecutive IGNITION_ON cycles with temp=115 (critical overheat each time).
**Expected:** Fault persists 3 cycles → marked persistent → STATE transitions to SAFE on cycle 3.
**Actual:** Cycle 1: DEGRADED. Cycle 2: DEGRADED (overheat count=2). Cycle 3: `Critical Overheat PERSISTENT for 3 consecutive cycles!`. STATE CHANGE: DEGRADED→SAFE. `⚠ PERSISTENT FAULT THRESHOLD REACHED -> STATE = SAFE`.
**Result: PASS**

---

### TC12 — Recovery After Reset
**Input:** After auto-reset, clean OFF→ACC→IGNITION_ON with speed=80, temp=75, gear=2.
**Expected:** No faults, no warnings, STATE=NORMAL.
**Actual:** All inputs validated OK. MODE: ACC→IGNITION_ON. WARNS=[NONE]. FAULTS=[NONE]. STATE=NORMAL.
**Result: PASS**

---

## Overall Result

**12 / 12 PASSED**

All fault conditions triggered correctly. All state transitions matched expected behaviour. Persistence threshold and SAFE latch both functioned as specified.
