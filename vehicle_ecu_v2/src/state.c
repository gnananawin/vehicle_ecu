#include <stdio.h>
#include <string.h>
#include "state.h"
#include "log.h"
#include "fault.h"

static bool s_safe_latch = false;

void state_init(VehicleStatus_t *status)
{
    status->system_state   = STATE_NORMAL;
    status->previous_state = STATE_NORMAL;
    s_safe_latch           = false;
    log_event(LOG_INFO, "STATE", "State manager initialized: NORMAL");
}

void evaluate_system_state(VehicleStatus_t     *status,
                           const FaultStatus_t *faults)
{
    SystemState_t new_state;
    const char   *reason = "No change";

    /* MODE_FAULT always forces SAFE */
    if (status->current_mode == MODE_FAULT)
    {
        if (!s_safe_latch)
        {
            s_safe_latch = true;
        }
    }

    /* Invalid gear immediately latches SAFE (mandatory TC5 requirement) */
    if (fault_is_active(faults, FAULT_INVALID_GEAR))
    {
        if (!s_safe_latch)
        {
            s_safe_latch = true;
        }
    }

    /* Once SAFE, stay SAFE until state_reset() is called */
    if (s_safe_latch)
    {
        if (status->system_state != STATE_SAFE)
        {
            status->previous_state = status->system_state;
            log_state_change(status->system_state, STATE_SAFE,
                             "System entered FAULT mode - latched SAFE");
            status->system_state = STATE_SAFE;
        }
        else
        {
            log_event(LOG_CRITICAL, "STATE",
                      "System locked in SAFE state. Manual reset required.");
        }
        return;
    }

    /* Normal evaluation */
    if (faults->persistent_fault_count >= FAULT_SAFE_STATE_THRESHOLD)
    {
        new_state    = STATE_SAFE;
        reason       = "Multiple persistent faults reached SAFE threshold";
        s_safe_latch = true;
    }
    else if (faults->active_flags != 0U)
    {
        new_state = STATE_DEGRADED;
        reason    = "Active faults present";
    }
    else
    {
        new_state = STATE_NORMAL;
        reason    = "No active faults";
    }

    if (new_state != status->system_state)
    {
        status->previous_state = status->system_state;
        log_state_change(status->system_state, new_state, reason);
        status->system_state = new_state;
    }
}

/* Reset everything to clean OFF state (models a power-cycle / workshop reset) */
void state_reset(VehicleStatus_t *status, FaultStatus_t *faults)
{
    log_event(LOG_INFO, "STATE", "System reset requested.");
    s_safe_latch             = false;
    status->fault_mode_latch = false;
    status->speed_in_acc     = false;
    fault_reset_all(faults);
    status->previous_state = status->system_state;
    status->system_state   = STATE_NORMAL;
    status->current_mode   = MODE_OFF;
    status->previous_mode  = MODE_OFF;
    log_state_change(status->previous_state, STATE_NORMAL, "Manual system reset");
    log_event(LOG_INFO, "MODE", "Mode reset to OFF after manual system reset.");
    log_event(LOG_INFO, "STATE",
              "Recovery complete: all fault counters cleared. "
              "Next clean cycle will confirm STATE_NORMAL.");
}

const char *state_to_string(SystemState_t state)
{
    const char *result;
    switch (state)
    {
        case STATE_NORMAL:   result = "NORMAL";   break;
        case STATE_DEGRADED: result = "DEGRADED"; break;
        case STATE_SAFE:     result = "SAFE";      break;
        default:             result = "UNKNOWN";   break;
    }
    return result;
}
