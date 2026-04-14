# Vehicle ECU — Console Logs
Representative output excerpts from scenario mode run (v1.3.0).

---

## 1. Normal Execution (TC1 — Normal Start Sequence)

```
══════════════════════════════════════════════════════════════
  TEST SCENARIO  1: TC1a: Normal Start - OFF mode
══════════════════════════════════════════════════════════════
  [INFO    ] [INPUT   ] All inputs validated OK.
──────────────────────────────────────────────────────────────
  [CYCLE: 1]
  [INPUT]   Mode=OFF           Speed=0     Temp=20    Gear=0

  CYCLE #1 SUMMARY
  ├─ INPUTS  Speed:    0 kph | Temp:   20 degC | Gear: 0 | Req.Mode: OFF
  ├─ MODE    Current: OFF          | Previous: OFF
  ├─ STATE   NORMAL   | Temp Status: NORMAL | Overspeed: NO | Gear OK: YES
  ├─ WARNS  [ NONE ]
  └─ FAULTS [ NONE ]

══════════════════════════════════════════════════════════════
  TEST SCENARIO  3: TC1c: Normal Start - ACC -> IGNITION_ON
══════════════════════════════════════════════════════════════
  [INFO    ] [INPUT   ] All inputs validated OK.
  [INFO    ] [MODE    ] Mode transition: ACC --> IGNITION_ON
──────────────────────────────────────────────────────────────
  [CYCLE: 3]
  [INPUT]   Mode=IGNITION_ON   Speed=40    Temp=80    Gear=1
  [MODE]    ACC -> IGNITION_ON

  CYCLE #3 SUMMARY
  ├─ INPUTS  Speed:   40 kph | Temp:   80 degC | Gear: 1 | Req.Mode: IGNITION_ON
  ├─ MODE    Current: IGNITION_ON  | Previous: ACC
  ├─ STATE   NORMAL   | Temp Status: NORMAL | Overspeed: NO | Gear OK: YES
  ├─ WARNS  [ NONE ]
  └─ FAULTS [ NONE ]
```

---

## 2. Warnings (TC3 — High Temperature)

```
══════════════════════════════════════════════════════════════
  TEST SCENARIO  5: TC3: High Temperature (temp=100 degC)
══════════════════════════════════════════════════════════════
  [INFO    ] [INPUT   ] All inputs validated OK.
  [WARNING ] [CONTROL ] High temperature warning: 100 degC
                        (high threshold: 95 degC) [WARNING only]
  [INFO    ] [CONTROL ] Overspeed condition cleared.
  [FAULT   ] Overspeed              CLEARED
  [STATE   ] [STATE CHANGE] DEGRADED -> NORMAL  | Reason: No active faults
──────────────────────────────────────────────────────────────
  CYCLE #5 SUMMARY
  ├─ STATE   NORMAL   | Temp Status: HIGH | Overspeed: NO | Gear OK: YES
  ├─ WARNS  [ HIGH_TEMP(100 degC) ]
  └─ FAULTS [ NONE ]
```

---

## 3. Faults (TC4 — Critical Overheat, TC2 — Overspeed, TC5 — Invalid Gear)

```
══════════════════════════════════════════════════════════════
  TEST SCENARIO  6: TC4: Critical Overheat (temp=115 degC)
══════════════════════════════════════════════════════════════
  [FAULT   ] Critical Overheat      ACTIVATED  | Total occurrences: 1
  [CRITICAL] [CONTROL ] [FAULT] Critical Overheat: 115 degC
                                (threshold: 110 degC) [Severity: CRITICAL]
  [STATE   ] [STATE CHANGE] NORMAL -> DEGRADED  | Reason: Active faults present
──────────────────────────────────────────────────────────────
  CYCLE #6 SUMMARY
  ├─ STATE   DEGRADED | Temp Status: CRITICAL | Overspeed: NO | Gear OK: YES
  ├─ WARNS  [ NONE ]
  └─ FAULTS [ Critical Overheat(1) ]

══════════════════════════════════════════════════════════════
  TEST SCENARIO  4: TC2: Overspeed (speed=130 kph)
══════════════════════════════════════════════════════════════
  [FAULT   ] Overspeed              ACTIVATED  | Total occurrences: 1
  [ERROR   ] [CONTROL ] [FAULT] Overspeed detected: 130 kph
                                (limit: 120 kph) [Severity: MAJOR]
──────────────────────────────────────────────────────────────
  CYCLE #4 SUMMARY
  ├─ STATE   DEGRADED | Temp Status: NORMAL | Overspeed: YES | Gear OK: YES
  ├─ WARNS  [ OVERSPEED(130 kph) ]
  └─ FAULTS [ Overspeed(1) ]
```

---

## 4. State Transitions

### NORMAL → SAFE (TC5 — Invalid Gear, immediate latch)
```
  [WARNING ] [INPUT   ] Gear 9 out of range [0-5]. FAULT_INVALID_GEAR will be raised.
  [FAULT   ] INVALID_GEAR           ACTIVATED  | Total occurrences: 1
  [ERROR   ] [CONTROL ] [FAULT] Invalid gear 9 in IGNITION_ON mode
                                (valid range: 0-5) [Severity: MAJOR]
                                -> Safe state latch will trigger
  [STATE   ] [STATE CHANGE] DEGRADED -> SAFE  |
               Reason: System entered FAULT mode - latched SAFE

  [STATE CHANGE] DEGRADED -> SAFE
  [PRIMARY FAULT] INVALID_GEAR

  ╔══════════════════════════════════════════════════════════════╗
  ║                  *** SYSTEM HALTED ***                       ║
  ║  Persistent fault threshold reached. STATE = SAFE.           ║
  ╚══════════════════════════════════════════════════════════════╝
```

### DEGRADED → SAFE (TC11 — Persistent Overheat, cycle 3/3)
```
  [PERSIST ] Critical Overheat      PERSISTENT for 3 consecutive cycles!
  [STATE   ] [STATE CHANGE] DEGRADED -> SAFE  |
               Reason: Multiple persistent faults reached SAFE threshold

  CYCLE #5 SUMMARY
  ├─ STATE   SAFE     | Temp Status: CRITICAL
  └─ FAULTS [ Critical Overheat(3)[PERSISTENT] ]
         ⚠ PERSISTENT FAULTS: 1 active
  ⚠ PERSISTENT FAULT THRESHOLD REACHED -> STATE = SAFE
    System halted. Restart or manual reset required.
```

### SAFE → NORMAL (Auto-reset between test groups)
```
  [AUTO-RESET] Resetting for remaining test scenarios...
  [INFO    ] [STATE   ] System reset requested.
  [INFO    ] [FAULT   ] All faults reset by system reset request.
  [STATE   ] [STATE CHANGE] SAFE -> NORMAL  | Reason: Manual system reset
  [INFO    ] [MODE    ] Mode reset to OFF after manual system reset.
  [INFO    ] [STATE   ] Recovery complete: all fault counters cleared.
```

### Normal MODE transitions
```
  [INFO    ] [MODE    ] Mode transition: OFF --> ACC
  [INFO    ] [MODE    ] Mode transition: ACC --> IGNITION_ON
  [INFO    ] [MODE    ] Mode transition: IGNITION_ON --> ACC
  [ERROR   ] [MODE    ] Illegal transition: OFF --> IGNITION_ON (FORCING FAULT MODE)
```

---

## 5. TC10 — Multiple Simultaneous Faults

```
══════════════════════════════════════════════════════════════
  TEST SCENARIO 17: TC10: Multi-fault: Overspeed+Critical Overheat+Invalid Gear
══════════════════════════════════════════════════════════════
  [FAULT   ] Critical Overheat      ACTIVATED  | Total occurrences: 1
  [CRITICAL] [CONTROL ] [FAULT] Critical Overheat: 120 degC [Severity: CRITICAL]
  [FAULT   ] INVALID_GEAR           ACTIVATED  | Total occurrences: 1
  [ERROR   ] [CONTROL ] [FAULT] Invalid gear 9 in IGNITION_ON mode [Severity: MAJOR]
  [FAULT   ] Overspeed              ACTIVATED  | Total occurrences: 1
  [ERROR   ] [CONTROL ] [FAULT] Overspeed detected: 140 kph [Severity: MAJOR]
  [STATE   ] [STATE CHANGE] NORMAL -> SAFE

  [FAULT]   Critical Overheat detected
  [FAULT]   INVALID_GEAR detected
  [FAULT]   Overspeed detected
  [PRIMARY FAULT] Critical Overheat

  CYCLE #3 SUMMARY
  ├─ STATE   SAFE     | Temp Status: CRITICAL | Overspeed: YES | Gear OK: NO
  ├─ WARNS  [ OVERSPEED(140 kph) ]
  └─ FAULTS [ Critical Overheat(1) INVALID_GEAR(1) Overspeed(1) ]
```
