#include <string.h>
#include "fault.h"
#include "log.h"

/* Bit mask lookup indexed by FaultType_t */
const uint8_t FAULT_BIT_MASK[FAULT_COUNT] =
{
    [FAULT_CRITICAL_OVERHEAT] = FAULT_BIT_CRITICAL_OVERHEAT,
    [FAULT_INVALID_GEAR]      = FAULT_BIT_INVALID_GEAR,
    [FAULT_OVERSPEED]         = FAULT_BIT_OVERSPEED,
    [FAULT_INVALID_MODE]      = FAULT_BIT_INVALID_MODE,
    [FAULT_SPEED_IN_OFF]      = FAULT_BIT_SPEED_IN_OFF,
    [FAULT_SPEED_IN_ACC]      = FAULT_BIT_SPEED_IN_ACC
};

static const char * const FAULT_NAMES[FAULT_COUNT] =
{
    [FAULT_CRITICAL_OVERHEAT] = "Critical Overheat",
    [FAULT_INVALID_GEAR]      = "INVALID_GEAR",
    [FAULT_OVERSPEED]         = "Overspeed",
    [FAULT_INVALID_MODE]      = "INVALID_MODE",
    [FAULT_SPEED_IN_OFF]      = "SPEED_IN_OFF",
    [FAULT_SPEED_IN_ACC]      = "Speed in ACC mode"
};

void fault_init(FaultStatus_t *faults)
{
    uint8_t i;

    faults->active_flags           = 0U;
    faults->persistent_fault_count = 0U;
    faults->has_primary_fault      = false;
    faults->primary_fault          = FAULT_COUNT;

    for (i = 0U; i < (uint8_t)FAULT_COUNT; i++)
    {
        faults->counters[i]          = 0U;
        faults->persistent_counts[i] = 0U;
        faults->is_persistent[i]     = false;
    }
}

void set_fault(FaultStatus_t *faults, FaultType_t fault)
{
    bool already_active;

    if (fault >= FAULT_COUNT) { return; }

    already_active = FAULT_IS_SET(faults->active_flags, FAULT_BIT_MASK[fault]);

    FAULT_SET(faults->active_flags, FAULT_BIT_MASK[fault]);
    faults->counters[fault]++;

    if (!already_active)
    {
        log_fault_set(fault, faults->counters[fault]);
    }
}

void clear_fault(FaultStatus_t *faults, FaultType_t fault)
{
    bool was_active;

    if (fault >= FAULT_COUNT) { return; }

    was_active = FAULT_IS_SET(faults->active_flags, FAULT_BIT_MASK[fault]);

    FAULT_CLEAR(faults->active_flags, FAULT_BIT_MASK[fault]);

    if (was_active)
    {
        log_fault_cleared(fault);
    }
}

void increment_fault_counter(FaultStatus_t *faults, FaultType_t fault)
{
    if (fault >= FAULT_COUNT) { return; }
    faults->counters[fault]++;
}

void update_fault_status(FaultStatus_t *faults)
{
    uint8_t i;
    uint8_t persistent_count = 0U;

    faults->has_primary_fault = false;
    faults->primary_fault     = FAULT_COUNT;

    for (i = 0U; i < (uint8_t)FAULT_COUNT; i++)
    {
        if (FAULT_IS_SET(faults->active_flags, FAULT_BIT_MASK[i]))
        {
            /* First active fault = highest priority (lowest index) */
            if (!faults->has_primary_fault)
            {
                faults->has_primary_fault = true;
                faults->primary_fault     = (FaultType_t)i;
            }

            faults->persistent_counts[i]++;

            if (faults->persistent_counts[i] >= FAULT_PERSISTENT_THRESHOLD)
            {
                if (!faults->is_persistent[i])
                {
                    faults->is_persistent[i] = true;
                    log_fault_persistent((FaultType_t)i,
                                         faults->persistent_counts[i]);
                }
                persistent_count++;
            }
        }
        else
        {
            /* Fault cleared: reset consecutive counter */
            faults->persistent_counts[i] = 0U;
            faults->is_persistent[i]     = false;
        }
    }

    faults->persistent_fault_count = persistent_count;
}

bool fault_is_active(const FaultStatus_t *faults, FaultType_t fault)
{
    if (fault >= FAULT_COUNT) { return false; }
    return FAULT_IS_SET(faults->active_flags, FAULT_BIT_MASK[fault]);
}

bool fault_is_persistent(const FaultStatus_t *faults, FaultType_t fault)
{
    if (fault >= FAULT_COUNT) { return false; }
    return faults->is_persistent[fault];
}

void fault_reset_all(FaultStatus_t *faults)
{
    fault_init(faults);
    log_event(LOG_INFO, "FAULT", "All faults reset by system reset request.");
}

const char *fault_name(FaultType_t fault)
{
    if (fault >= FAULT_COUNT) { return "UNKNOWN"; }
    return FAULT_NAMES[fault];
}
