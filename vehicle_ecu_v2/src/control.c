#include <stdio.h>
#include <string.h>
#include "control.h"
#include "fault.h"
#include "mode.h"
#include "input.h"
#include "log.h"

void run_control_checks(const VehicleInput_t *input,
                        VehicleStatus_t      *status,
                        FaultStatus_t        *faults)
{
    /* Skip all checks if system is already in FAULT mode */
    if (status->current_mode == MODE_FAULT)
    {
        log_event(LOG_CRITICAL, "CONTROL",
                  "System in FAULT mode. Sensor checks suspended.");
        return;
    }

    /* Run checks in priority order */
    check_temperature(input, status, faults);   /* Priority 1: always live */
    check_gear(input, status, faults);           /* Priority 2: all modes   */
    check_overspeed(input, status, faults);      /* Priority 3: IGNITION_ON */
    check_speed_in_acc(input, status, faults);   /* Priority 4: ACC mode    */
    check_speed_in_off(input, status, faults);   /* Priority 5: OFF mode    */
}

void check_overspeed(const VehicleInput_t *input,
                     VehicleStatus_t      *status,
                     FaultStatus_t        *faults)
{
    char msg[96];

    if (input->speed_kph > SPEED_OVERSPEED_KPH)
    {
        status->overspeed = true;
        set_fault(faults, FAULT_OVERSPEED);

        (void)snprintf(msg, sizeof(msg),
                       "[FAULT] Overspeed detected: %d kph (limit: %d kph) "
                       "[Severity: MAJOR]",
                       input->speed_kph, SPEED_OVERSPEED_KPH);
        log_event(LOG_ERROR, "CONTROL", msg);
    }
    else
    {
        if (status->overspeed)
        {
            log_event(LOG_INFO, "CONTROL", "Overspeed condition cleared.");
        }
        status->overspeed = false;
        clear_fault(faults, FAULT_OVERSPEED);
    }
}

void check_temperature(const VehicleInput_t *input,
                       VehicleStatus_t      *status,
                       FaultStatus_t        *faults)
{
    char msg[120];

    if (input->temperature_c > TEMP_CRITICAL_CELSIUS)
    {
        status->temp_status = TEMP_STATUS_CRITICAL;
        set_fault(faults, FAULT_CRITICAL_OVERHEAT);

        (void)snprintf(msg, sizeof(msg),
                       "[FAULT] Critical Overheat: %d degC "
                       "(threshold: %d degC) [Severity: CRITICAL]",
                       input->temperature_c, TEMP_CRITICAL_CELSIUS);
        log_event(LOG_CRITICAL, "CONTROL", msg);
    }
    else if (input->temperature_c > TEMP_HIGH_CELSIUS)
    {
        status->temp_status = TEMP_STATUS_HIGH;
        clear_fault(faults, FAULT_CRITICAL_OVERHEAT);

        (void)snprintf(msg, sizeof(msg),
                       "High temperature warning: %d degC "
                       "(high threshold: %d degC) [WARNING only]",
                       input->temperature_c, TEMP_HIGH_CELSIUS);
        log_event(LOG_WARNING, "CONTROL", msg);
    }
    else
    {
        if (status->temp_status != TEMP_STATUS_NORMAL)
        {
            log_event(LOG_INFO, "CONTROL", "Temperature returned to normal range.");
        }
        status->temp_status = TEMP_STATUS_NORMAL;
        clear_fault(faults, FAULT_CRITICAL_OVERHEAT);
    }
}

void check_gear(const VehicleInput_t *input,
                VehicleStatus_t      *status,
                FaultStatus_t        *faults)
{
    char msg[120];

    /* gear > GEAR_MAX is invalid in any mode; triggers immediate SAFE latch */
    if (input->gear > GEAR_MAX)
    {
        status->gear_valid = false;
        set_fault(faults, FAULT_INVALID_GEAR);

        (void)snprintf(msg, sizeof(msg),
                       "[FAULT] Invalid gear %u in %s mode "
                       "(valid range: %u-%u) [Severity: MAJOR] "
                       "-> Safe state latch will trigger",
                       input->gear,
                       mode_to_string(status->current_mode),
                       GEAR_MIN, GEAR_MAX);
        log_event(LOG_ERROR, "CONTROL", msg);
    }
    else
    {
        if (!status->gear_valid)
        {
            log_event(LOG_INFO, "CONTROL", "Gear returned to valid range.");
        }
        status->gear_valid = true;
        clear_fault(faults, FAULT_INVALID_GEAR);
    }
}

void check_speed_in_off(const VehicleInput_t *input,
                        VehicleStatus_t      *status,
                        FaultStatus_t        *faults)
{
    char msg[96];

    if ((status->current_mode == MODE_OFF) && (input->speed_kph > 0))
    {
        status->speed_in_off = true;
        set_fault(faults, FAULT_SPEED_IN_OFF);

        (void)snprintf(msg, sizeof(msg),
                       "Speed %d kph detected in OFF mode [Severity: WARNING]",
                       input->speed_kph);
        log_event(LOG_WARNING, "CONTROL", msg);
    }
    else
    {
        if (status->speed_in_off)
        {
            log_event(LOG_INFO, "CONTROL", "Speed-in-OFF condition cleared.");
        }
        status->speed_in_off = false;
        clear_fault(faults, FAULT_SPEED_IN_OFF);
    }
}

void check_speed_in_acc(const VehicleInput_t *input,
                        VehicleStatus_t      *status,
                        FaultStatus_t        *faults)
{
    ECU_UNUSED(input);

    if (status->speed_in_acc)
    {
        set_fault(faults, FAULT_SPEED_IN_ACC);
        log_event(LOG_ERROR, "CONTROL",
                  "[FAULT] Speed in ACC mode: ACC is accessory-only. "
                  "Speed input rejected and zeroed. [Severity: MAJOR]");
    }
    else
    {
        clear_fault(faults, FAULT_SPEED_IN_ACC);
    }
}
