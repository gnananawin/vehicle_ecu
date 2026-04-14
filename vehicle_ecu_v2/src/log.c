#include <stdio.h>
#include <string.h>
#include "log.h"
#include "input.h"
#include "state.h"
#include "fault.h"

static CycleLogBuffer_t s_cycle_buf;

static const char *log_level_str(LogLevel_t level)
{
    const char *result;
    switch (level)
    {
        case LOG_INFO:     result = LOG_COLOR_CYAN    "[INFO    ]" LOG_COLOR_RESET; break;
        case LOG_WARNING:  result = LOG_COLOR_YELLOW  "[WARNING ]" LOG_COLOR_RESET; break;
        case LOG_ERROR:    result = LOG_COLOR_RED     "[ERROR   ]" LOG_COLOR_RESET; break;
        case LOG_FAULT:    result = LOG_COLOR_MAGENTA "[FAULT   ]" LOG_COLOR_RESET; break;
        case LOG_CRITICAL: result = LOG_COLOR_RED     "[CRITICAL]" LOG_COLOR_RESET; break;
        default:           result =                   "[UNKNOWN ]";                 break;
    }
    return result;
}

/* Append plain text (no ANSI codes) to the cycle buffer */
static void buf_append(const char *text)
{
    uint32_t remaining;
    uint32_t len;

    if (s_cycle_buf.length >= (CYCLE_LOG_BUFFER_SIZE - 1U))
    {
        return;
    }

    remaining = (CYCLE_LOG_BUFFER_SIZE - 1U) - s_cycle_buf.length;
    len = (uint32_t)strlen(text);
    if (len > remaining) { len = remaining; }
    (void)memcpy(&s_cycle_buf.data[s_cycle_buf.length], text, (size_t)len);
    s_cycle_buf.length += len;
    s_cycle_buf.data[s_cycle_buf.length] = '\0';
}

void log_init(void)
{
    (void)memset(&s_cycle_buf, 0, sizeof(s_cycle_buf));
    log_banner();
}

void log_banner(void)
{
    printf("\n");
    printf(LOG_COLOR_CYAN);
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║         VEHICLE ECU SIMULATION SYSTEM v%u.%u.%u               ║\n",
           ECU_SW_VERSION_MAJOR, ECU_SW_VERSION_MINOR, ECU_SW_VERSION_PATCH);
    printf("║   MISRA-C:2012 / AUTOSAR Compliant Embedded C Scheduler      ║\n");
    printf("║   Output log: ecu_last_cycle.log (last cycle only)           ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf(LOG_COLOR_RESET);
    printf("\n");
}

void log_separator(void)
{
    printf(LOG_COLOR_BLUE
           "──────────────────────────────────────────────────────────────\n"
           LOG_COLOR_RESET);
}

void log_scenario_header(uint32_t scenario_num, const char *description)
{
    printf("\n");
    printf(LOG_COLOR_WHITE
           "══════════════════════════════════════════════════════════════\n"
           LOG_COLOR_RESET);
    printf(LOG_COLOR_WHITE "  TEST SCENARIO %2u: %s\n" LOG_COLOR_RESET,
           scenario_num, description);
    printf(LOG_COLOR_WHITE
           "══════════════════════════════════════════════════════════════\n"
           LOG_COLOR_RESET);
}

void log_event(LogLevel_t level, const char *module, const char *message)
{
    printf("  %s [%-8s] %s\n", log_level_str(level), module, message);
}

void log_illegal_transition(VehicleMode_t from, VehicleMode_t to)
{
    printf("  " LOG_COLOR_RED "[ERROR   ]" LOG_COLOR_RESET
           " [MODE    ] Illegal transition: %s --> %s  "
           LOG_COLOR_RED "(FORCING FAULT MODE)" LOG_COLOR_RESET "\n",
           mode_to_string(from), mode_to_string(to));
}

void log_state_change(SystemState_t from, SystemState_t to, const char *reason)
{
    const char *color = LOG_COLOR_GREEN;
    if (to == STATE_DEGRADED) { color = LOG_COLOR_YELLOW; }
    if (to == STATE_SAFE)     { color = LOG_COLOR_RED;    }

    printf("  " LOG_COLOR_CYAN "[STATE   ]" LOG_COLOR_RESET
           " [STATE CHANGE] %s -> %s%s" LOG_COLOR_RESET "  | Reason: %s\n",
           state_to_string(from), color, state_to_string(to), reason);
}

void log_fault_set(FaultType_t fault, uint32_t counter)
{
    printf("  " LOG_COLOR_MAGENTA "[FAULT   ]" LOG_COLOR_RESET
           " %-22s ACTIVATED  | Total occurrences: %u\n",
           fault_name(fault), counter);
}

void log_fault_cleared(FaultType_t fault)
{
    printf("  " LOG_COLOR_GREEN "[FAULT   ]" LOG_COLOR_RESET
           " %-22s CLEARED\n", fault_name(fault));
}

void log_fault_persistent(FaultType_t fault, uint32_t cycles)
{
    printf("  " LOG_COLOR_RED "[PERSIST ]" LOG_COLOR_RESET
           " %-22s PERSISTENT for %u consecutive cycles!\n",
           fault_name(fault), cycles);
}

void log_cycle_summary(const VehicleInput_t  *input,
                       const VehicleStatus_t *status,
                       const FaultStatus_t   *faults)
{
    uint8_t     i;
    bool        any_fault   = false;
    bool        any_warning = false;
    const char *state_color;

    log_separator();

    printf(LOG_COLOR_WHITE "\n  [CYCLE: %u]\n" LOG_COLOR_RESET,
           status->cycle_count);

    printf("  [INPUT]   Mode=%-12s  Speed=%-4d  Temp=%-4d  Gear=%u\n",
           mode_to_string(input->requested_mode),
           input->speed_kph,
           input->temperature_c,
           input->gear);

    for (i = 0U; i < (uint8_t)FAULT_COUNT; i++)
    {
        if (fault_is_active(faults, (FaultType_t)i))
        {
            any_fault = true;
            printf(LOG_COLOR_RED "  [FAULT]   %s detected\n" LOG_COLOR_RESET,
                   fault_name((FaultType_t)i));
        }
    }

    if (faults->has_primary_fault && any_fault)
    {
        printf(LOG_COLOR_RED
               "  [PRIMARY FAULT] %s\n"
               LOG_COLOR_RESET,
               fault_name(faults->primary_fault));
    }

    if (status->system_state != status->previous_state)
    {
        const char *sc_color = LOG_COLOR_GREEN;
        if (status->system_state == STATE_DEGRADED) { sc_color = LOG_COLOR_YELLOW; }
        if (status->system_state == STATE_SAFE)     { sc_color = LOG_COLOR_RED;    }

        printf(LOG_COLOR_CYAN "  [STATE CHANGE] " LOG_COLOR_RESET
               "%s -> %s%s" LOG_COLOR_RESET "\n",
               state_to_string(status->previous_state),
               sc_color,
               state_to_string(status->system_state));
    }

    if (status->current_mode != status->previous_mode)
    {
        printf(LOG_COLOR_CYAN "  [MODE] " LOG_COLOR_RESET
               "%s -> %s\n",
               mode_to_string(status->previous_mode),
               mode_to_string(status->current_mode));
    }

    printf("\n");

    if (status->system_state == STATE_NORMAL)        { state_color = LOG_COLOR_GREEN;  }
    else if (status->system_state == STATE_DEGRADED) { state_color = LOG_COLOR_YELLOW; }
    else                                             { state_color = LOG_COLOR_RED;    }

    printf(LOG_COLOR_WHITE "  CYCLE #%u SUMMARY\n" LOG_COLOR_RESET,
           status->cycle_count);

    printf("  ├─ " LOG_COLOR_CYAN "INPUTS" LOG_COLOR_RESET
           "  Speed: %4d kph | Temp: %4d degC | Gear: %u | Req.Mode: %s\n",
           input->speed_kph,
           input->temperature_c,
           input->gear,
           mode_to_string(input->requested_mode));

    printf("  ├─ " LOG_COLOR_CYAN "MODE  " LOG_COLOR_RESET
           "  Current: %-12s | Previous: %-12s\n",
           mode_to_string(status->current_mode),
           mode_to_string(status->previous_mode));

    printf("  ├─ " LOG_COLOR_CYAN "STATE " LOG_COLOR_RESET
           "  %s%-8s" LOG_COLOR_RESET
           " | Temp Status: %s | Overspeed: %s | Gear OK: %s\n",
           state_color, state_to_string(status->system_state),
           (status->temp_status == TEMP_STATUS_NORMAL)
               ? LOG_COLOR_GREEN  "NORMAL"    LOG_COLOR_RESET
               : (status->temp_status == TEMP_STATUS_HIGH)
                   ? LOG_COLOR_YELLOW "HIGH"  LOG_COLOR_RESET
                   : LOG_COLOR_RED    "CRITICAL" LOG_COLOR_RESET,
           status->overspeed
               ? LOG_COLOR_RED "YES" LOG_COLOR_RESET
               : LOG_COLOR_GREEN "NO" LOG_COLOR_RESET,
           status->gear_valid
               ? LOG_COLOR_GREEN "YES" LOG_COLOR_RESET
               : LOG_COLOR_RED "NO"  LOG_COLOR_RESET);

    if (status->temp_status == TEMP_STATUS_HIGH) { any_warning = true; }
    if (status->speed_in_off)                    { any_warning = true; }
    if (status->overspeed)                       { any_warning = true; }

    printf("  ├─ " LOG_COLOR_CYAN "WARNS " LOG_COLOR_RESET " [");
    if (status->temp_status == TEMP_STATUS_HIGH)
    {
        printf(LOG_COLOR_YELLOW " HIGH_TEMP(%d degC)" LOG_COLOR_RESET,
               input->temperature_c);
    }
    if (status->speed_in_off)
    {
        printf(LOG_COLOR_YELLOW " SPEED_IN_OFF" LOG_COLOR_RESET);
    }
    if (status->overspeed)
    {
        printf(LOG_COLOR_YELLOW " OVERSPEED(%d kph)" LOG_COLOR_RESET,
               input->speed_kph);
    }
    if (!any_warning)
    {
        printf(LOG_COLOR_GREEN " NONE" LOG_COLOR_RESET);
    }
    printf(" ]\n");

    printf("  └─ " LOG_COLOR_CYAN "FAULTS" LOG_COLOR_RESET " [");
    for (i = 0U; i < (uint8_t)FAULT_COUNT; i++)
    {
        if (fault_is_active(faults, (FaultType_t)i))
        {
            printf(LOG_COLOR_RED " %s(%u)" LOG_COLOR_RESET,
                   fault_name((FaultType_t)i),
                   faults->counters[i]);
            if (faults->is_persistent[i])
            {
                printf(LOG_COLOR_RED "[PERSISTENT]" LOG_COLOR_RESET);
            }
        }
    }
    if (!any_fault)
    {
        printf(LOG_COLOR_GREEN " NONE" LOG_COLOR_RESET);
    }
    printf(" ]\n");

    if (faults->persistent_fault_count > 0U)
    {
        printf("         " LOG_COLOR_RED
               "⚠ PERSISTENT FAULTS: %u active\n" LOG_COLOR_RESET,
               faults->persistent_fault_count);
    }

    if (status->fault_mode_latch)
    {
        printf(LOG_COLOR_RED
               "\n  ⚠ SYSTEM IN FAULT MODE - Manual reset required\n"
               LOG_COLOR_RESET);
    }

    if ((status->system_state == STATE_SAFE) &&
        (faults->persistent_fault_count >= FAULT_SAFE_STATE_THRESHOLD))
    {
        printf(LOG_COLOR_RED
               "\n  ⚠ PERSISTENT FAULT THRESHOLD REACHED -> STATE = SAFE\n"
               "    System halted. Restart or manual reset required.\n"
               LOG_COLOR_RESET);
    }

    printf("\n");

    log_file_flush_last_cycle(input, status, faults);
}

/* Write plain-text last-cycle summary to ecu_last_cycle.log (overwrite each cycle) */
void log_file_flush_last_cycle(const VehicleInput_t  *input,
                               const VehicleStatus_t *status,
                               const FaultStatus_t   *faults)
{
    FILE    *fp;
    uint8_t  i;
    bool     any_fault   = false;
    bool     any_warning = false;
    char     line[256];

    (void)memset(&s_cycle_buf, 0, sizeof(s_cycle_buf));
    s_cycle_buf.cycle_num = status->cycle_count;

    (void)snprintf(line, sizeof(line),
                   "============================================================\n"
                   "  VEHICLE ECU LAST CYCLE LOG  v%u.%u.%u\n"
                   "============================================================\n",
                   ECU_SW_VERSION_MAJOR,
                   ECU_SW_VERSION_MINOR,
                   ECU_SW_VERSION_PATCH);
    buf_append(line);

    (void)snprintf(line, sizeof(line),
                   "\n  [CYCLE: %u]\n", status->cycle_count);
    buf_append(line);

    (void)snprintf(line, sizeof(line),
                   "  [INPUT]   Mode=%-12s  Speed=%-4d  Temp=%-4d  Gear=%u\n",
                   mode_to_string(input->requested_mode),
                   input->speed_kph,
                   input->temperature_c,
                   input->gear);
    buf_append(line);

    for (i = 0U; i < (uint8_t)FAULT_COUNT; i++)
    {
        if (fault_is_active(faults, (FaultType_t)i))
        {
            any_fault = true;
            (void)snprintf(line, sizeof(line),
                           "  [FAULT]   %s detected\n",
                           fault_name((FaultType_t)i));
            buf_append(line);
        }
    }

    if (faults->has_primary_fault && any_fault)
    {
        (void)snprintf(line, sizeof(line),
                       "  [PRIMARY FAULT] %s\n",
                       fault_name(faults->primary_fault));
        buf_append(line);
    }

    if (status->system_state != status->previous_state)
    {
        (void)snprintf(line, sizeof(line),
                       "  [STATE CHANGE] %s -> %s\n",
                       state_to_string(status->previous_state),
                       state_to_string(status->system_state));
        buf_append(line);
    }

    if (status->current_mode != status->previous_mode)
    {
        (void)snprintf(line, sizeof(line),
                       "  [MODE] %s -> %s\n",
                       mode_to_string(status->previous_mode),
                       mode_to_string(status->current_mode));
        buf_append(line);
    }

    (void)snprintf(line, sizeof(line),
                   "\n  CYCLE #%u SUMMARY\n", status->cycle_count);
    buf_append(line);

    (void)snprintf(line, sizeof(line),
                   "  ├─ INPUTS   Speed: %4d kph | Temp: %4d degC | "
                   "Gear: %u | Mode: %s\n",
                   input->speed_kph, input->temperature_c,
                   input->gear, mode_to_string(input->requested_mode));
    buf_append(line);

    (void)snprintf(line, sizeof(line),
                   "  ├─ MODE     Current: %-12s | Previous: %-12s\n",
                   mode_to_string(status->current_mode),
                   mode_to_string(status->previous_mode));
    buf_append(line);

    (void)snprintf(line, sizeof(line),
                   "  ├─ STATE    %-8s | Temp: %s | Overspeed: %s | Gear OK: %s\n",
                   state_to_string(status->system_state),
                   (status->temp_status == TEMP_STATUS_NORMAL) ? "NORMAL  " :
                   (status->temp_status == TEMP_STATUS_HIGH)   ? "HIGH    " :
                                                                  "CRITICAL",
                   status->overspeed ? "YES" : "NO ",
                   status->gear_valid ? "YES" : "NO ");
    buf_append(line);

    buf_append("  ├─ WARNS  [");
    if (status->temp_status == TEMP_STATUS_HIGH)
    {
        (void)snprintf(line, sizeof(line), " HIGH_TEMP(%d degC)",
                       input->temperature_c);
        buf_append(line);
        any_warning = true;
    }
    if (status->speed_in_off) { buf_append(" SPEED_IN_OFF"); any_warning = true; }
    if (status->overspeed)
    {
        (void)snprintf(line, sizeof(line), " OVERSPEED(%d kph)",
                       input->speed_kph);
        buf_append(line);
        any_warning = true;
    }
    if (!any_warning) { buf_append(" NONE"); }
    buf_append(" ]\n");

    buf_append("  └─ FAULTS [");
    for (i = 0U; i < (uint8_t)FAULT_COUNT; i++)
    {
        if (fault_is_active(faults, (FaultType_t)i))
        {
            (void)snprintf(line, sizeof(line), " %s(%u)%s",
                           fault_name((FaultType_t)i),
                           faults->counters[i],
                           faults->is_persistent[i] ? "[PERSISTENT]" : "");
            buf_append(line);
        }
    }
    if (!any_fault) { buf_append(" NONE"); }
    buf_append(" ]\n");

    if (faults->persistent_fault_count > 0U)
    {
        (void)snprintf(line, sizeof(line),
                       "  PERSISTENT FAULTS: %u active\n",
                       faults->persistent_fault_count);
        buf_append(line);
    }

    if (status->system_state == STATE_SAFE)
    {
        buf_append("  ** SYSTEM STATE = SAFE - HALTED **\n");
    }

    buf_append("============================================================\n");

    fp = fopen(ECU_LOG_FILENAME, "w");
    if (fp != NULL)
    {
        (void)fwrite(s_cycle_buf.data, 1U, (size_t)s_cycle_buf.length, fp);
        (void)fclose(fp);
    }
}
