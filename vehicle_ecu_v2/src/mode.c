#include <stdio.h>
#include <string.h>
#include "mode.h"
#include "input.h"
#include "fault.h"
#include "log.h"

/*
 * Legal transition table (OFF/ACC/IGNITION_ON only; FAULT is handled separately).
 * OFF->ACC, ACC->IGNITION_ON, ACC->OFF, IGNITION_ON->ACC, IGNITION_ON->OFF allowed.
 * OFF->IGNITION_ON is NOT allowed (must go through ACC first).
 */
#define MODE_TABLE_SIZE  (3U)

static const bool LEGAL_TRANSITIONS[MODE_TABLE_SIZE][MODE_TABLE_SIZE] =
{
    /*             TO:  OFF    ACC    IGN    */
    /* FROM OFF  */   { true,  true,  false },
    /* FROM ACC  */   { true,  false, true  },
    /* FROM IGN  */   { true,  true,  false }
};

static void enter_fault_mode(VehicleStatus_t *status,
                              FaultStatus_t   *faults,
                              const char      *reason)
{
    char msg[128];

    if (status->current_mode == MODE_FAULT)
    {
        log_event(LOG_CRITICAL, "MODE",
                  "System already in FAULT mode. Latched until manual reset.");
        return;
    }

    (void)snprintf(msg, sizeof(msg),
                   "Entering FAULT mode: %s --> FAULT | Reason: %s",
                   mode_to_string(status->current_mode), reason);
    log_event(LOG_CRITICAL, "MODE", msg);

    set_fault(faults, FAULT_INVALID_MODE);

    status->previous_mode    = status->current_mode;
    status->current_mode     = MODE_FAULT;
    status->fault_mode_latch = true;
}

void mode_init(VehicleStatus_t *status)
{
    status->current_mode     = MODE_OFF;
    status->previous_mode    = MODE_OFF;
    status->fault_mode_latch = false;
    log_event(LOG_INFO, "MODE", "Mode controller initialized: OFF");
}

bool mode_transition_allowed(VehicleMode_t from, VehicleMode_t to)
{
    if (to == MODE_FAULT) { return true; }

    if (((uint8_t)from >= MODE_TABLE_SIZE) || ((uint8_t)to >= MODE_TABLE_SIZE))
    {
        return false;
    }
    return LEGAL_TRANSITIONS[(uint8_t)from][(uint8_t)to];
}

void update_mode(VehicleStatus_t      *status,
                 const VehicleInput_t *input,
                 FaultStatus_t        *faults)
{
    VehicleMode_t requested = input->requested_mode;

    if (status->fault_mode_latch)
    {
        log_event(LOG_CRITICAL, "MODE",
                  "FAULT mode latched. No mode change allowed until manual reset.");
        return;
    }

    if (requested == MODE_FAULT)
    {
        enter_fault_mode(status, faults, "Direct user input MODE=FAULT (3)");
        return;
    }

    if (requested == status->current_mode)
    {
        return;
    }

    if (mode_transition_allowed(status->current_mode, requested))
    {
        char msg[96];
        (void)snprintf(msg, sizeof(msg),
                       "Mode transition: %s --> %s",
                       mode_to_string(status->current_mode),
                       mode_to_string(requested));
        log_event(LOG_INFO, "MODE", msg);

        status->previous_mode = status->current_mode;
        status->current_mode  = requested;
    }
    else
    {
        char reason[96];
        (void)snprintf(reason, sizeof(reason),
                       "Illegal transition %s --> %s detected",
                       mode_to_string(status->current_mode),
                       mode_to_string(requested));

        log_illegal_transition(status->current_mode, requested);
        enter_fault_mode(status, faults, reason);
    }
}

/* Called by control logic when a critical condition forces FAULT */
void mode_force_fault(VehicleStatus_t *status,
                      FaultStatus_t   *faults,
                      const char      *reason)
{
    enter_fault_mode(status, faults, reason);
}
