#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "input.h"
#include "log.h"

static VehicleInput_t        s_last_good_input;
static InputMode_t           s_input_mode     = INPUT_MODE_INTERACTIVE;
static const TestScenario_t *s_scenarios      = NULL;
static uint32_t              s_scenario_count = 0U;
static uint32_t              s_scenario_index = 0U;

static VehicleMode_t parse_mode(int32_t raw)
{
    VehicleMode_t result;
    switch (raw)
    {
        case 0:  result = MODE_OFF;         break;
        case 1:  result = MODE_ACC;         break;
        case 2:  result = MODE_IGNITION_ON; break;
        case 3:  result = MODE_FAULT;       break;
        default: result = MODE_FAULT;       break;
    }
    return result;
}

void input_init(VehicleInput_t *input)
{
    s_last_good_input.speed_kph      = 0;
    s_last_good_input.temperature_c  = 20;
    s_last_good_input.gear           = 0U;
    s_last_good_input.requested_mode = MODE_OFF;
    s_last_good_input.input_valid    = true;

    *input = s_last_good_input;
}

void input_set_mode(InputMode_t mode)
{
    s_input_mode = mode;
}

void input_load_scenarios(const TestScenario_t *scenarios, uint32_t count)
{
    s_scenarios      = scenarios;
    s_scenario_count = count;
    s_scenario_index = 0U;
}

bool input_scenarios_done(void)
{
    return (s_scenario_index >= s_scenario_count);
}

void read_inputs(VehicleInput_t *input)
{
    int32_t raw_mode;

    if (s_input_mode == INPUT_MODE_SCENARIO)
    {
        if ((s_scenarios != NULL) && (s_scenario_index < s_scenario_count))
        {
            const TestScenario_t *sc = &s_scenarios[s_scenario_index];
            log_scenario_header(s_scenario_index + 1U, sc->description);

            input->speed_kph      = sc->speed_kph;
            input->temperature_c  = sc->temperature_c;
            input->gear           = sc->gear;
            input->requested_mode = sc->requested_mode;
            input->input_valid    = true;

            s_scenario_index++;
        }
        return;
    }

    /* Interactive mode: prompt user for each input */
    printf("\n" LOG_COLOR_CYAN
           "──── Enter Inputs (Ctrl+C to quit) ────\n"
           LOG_COLOR_RESET);

    printf("  Mode  [0=OFF  1=ACC  2=IGNITION_ON  3=FAULT]: ");
    if (scanf("%d", &raw_mode) != 1) { raw_mode = 0; }
    input->requested_mode = parse_mode(raw_mode);

    if (input->requested_mode == MODE_OFF)
    {
        input->speed_kph     = 0;
        input->temperature_c = 20;
        input->gear          = 0U;
        input->input_valid   = true;
        log_event(LOG_INFO, "INPUT",
                  "Mode OFF: safe defaults (speed=0, temp=20, gear=0).");
        return;
    }

    if (input->requested_mode == MODE_FAULT)
    {
        input->speed_kph     = 0;
        input->temperature_c = 20;
        input->gear          = 0U;
        input->input_valid   = true;
        log_event(LOG_CRITICAL, "INPUT",
                  "Mode FAULT (3) entered by user. System will enter FAULT mode.");
        return;
    }

    /* ACC: speed NOT accepted (engine off); temp and gear selector accepted */
    if (input->requested_mode == MODE_ACC)
    {
        input->speed_kph = 0;

        printf("  Temp  (degC) [%d-%d]: ", TEMP_MIN_CELSIUS, TEMP_MAX_CELSIUS);
        if (scanf("%d", &input->temperature_c) != 1)
        {
            input->temperature_c = s_last_good_input.temperature_c;
        }

        printf("  Gear  [%u-%u] (gear selector, engine off): ", GEAR_MIN, GEAR_MAX);
        if (scanf("%hhu", &input->gear) != 1)
        {
            input->gear = s_last_good_input.gear;
        }

        input->input_valid = true;
        log_event(LOG_INFO, "INPUT",
                  "Mode ACC: speed NOT accepted (engine off). Temp+Gear accepted.");
        return;
    }

    /* IGNITION_ON: all three inputs accepted */
    printf("  Speed (kph)  [%d-%d]: ", SPEED_MIN_KPH, SPEED_MAX_KPH);
    if (scanf("%d", &input->speed_kph) != 1)
    {
        input->speed_kph = s_last_good_input.speed_kph;
    }

    printf("  Temp  (degC) [%d-%d]: ", TEMP_MIN_CELSIUS, TEMP_MAX_CELSIUS);
    if (scanf("%d", &input->temperature_c) != 1)
    {
        input->temperature_c = s_last_good_input.temperature_c;
    }

    printf("  Gear  [%u-%u]: ", GEAR_MIN, GEAR_MAX);
    if (scanf("%hhu", &input->gear) != 1)
    {
        input->gear = s_last_good_input.gear;
    }

    input->input_valid = true;
}

void validate_inputs(VehicleInput_t *input, VehicleStatus_t *status)
{
    bool  all_valid = true;
    char  msg[128];

    /* OFF: enforce zero speed and gear */
    if (input->requested_mode == MODE_OFF)
    {
        if (input->speed_kph != 0)
        {
            (void)snprintf(msg, sizeof(msg),
                           "Speed %d rejected in OFF mode. Forced to 0.",
                           input->speed_kph);
            log_event(LOG_WARNING, "INPUT", msg);
            input->speed_kph = 0;
            all_valid = false;
        }
        if (input->gear != 0U)
        {
            (void)snprintf(msg, sizeof(msg),
                           "Gear %u rejected in OFF mode. Forced to 0.",
                           input->gear);
            log_event(LOG_WARNING, "INPUT", msg);
            input->gear = 0U;
            all_valid   = false;
        }
        status->speed_in_acc = false;
    }

    /* ACC: speed is illegal (engine off) -> sets FAULT_SPEED_IN_ACC flag */
    if (input->requested_mode == MODE_ACC)
    {
        if (input->speed_kph != 0)
        {
            (void)snprintf(msg, sizeof(msg),
                           "[FAULT] Speed %d kph received in ACC mode. "
                           "ACC is engine-off/accessory-only. "
                           "Speed input INVALID - forced to 0.",
                           input->speed_kph);
            log_event(LOG_ERROR, "INPUT", msg);
            status->speed_in_acc = true;
            input->speed_kph     = 0;
            all_valid            = false;
        }
        else
        {
            status->speed_in_acc = false;
        }
        /* Gear is valid in ACC; no rejection here */
    }

    /* Speed range check */
    if ((input->speed_kph < SPEED_MIN_KPH) || (input->speed_kph > SPEED_MAX_KPH))
    {
        (void)snprintf(msg, sizeof(msg),
                       "Speed %d out of range [%d-%d]. Using last good: %d",
                       input->speed_kph, SPEED_MIN_KPH, SPEED_MAX_KPH,
                       s_last_good_input.speed_kph);
        log_event(LOG_WARNING, "INPUT", msg);
        input->speed_kph = s_last_good_input.speed_kph;
        all_valid = false;
    }
    else
    {
        s_last_good_input.speed_kph = input->speed_kph;
    }

    /* Temperature range check */
    if ((input->temperature_c < TEMP_MIN_CELSIUS) ||
        (input->temperature_c > TEMP_MAX_CELSIUS))
    {
        (void)snprintf(msg, sizeof(msg),
                       "Temp %d out of range [%d-%d]. Using last good: %d",
                       input->temperature_c, TEMP_MIN_CELSIUS, TEMP_MAX_CELSIUS,
                       s_last_good_input.temperature_c);
        log_event(LOG_WARNING, "INPUT", msg);
        input->temperature_c = s_last_good_input.temperature_c;
        all_valid = false;
    }
    else
    {
        s_last_good_input.temperature_c = input->temperature_c;
    }

    /* Gear range check: invalid gear passes through so control.c can raise the fault */
    if (input->gear > GEAR_MAX)
    {
        (void)snprintf(msg, sizeof(msg),
                       "Gear %u out of range [%u-%u]. "
                       "FAULT_INVALID_GEAR will be raised.",
                       input->gear, GEAR_MIN, GEAR_MAX);
        log_event(LOG_WARNING, "INPUT", msg);
        all_valid = false;
    }
    else
    {
        s_last_good_input.gear = input->gear;
    }

    /* Mode check */
    if ((input->requested_mode != MODE_OFF)         &&
        (input->requested_mode != MODE_ACC)         &&
        (input->requested_mode != MODE_IGNITION_ON) &&
        (input->requested_mode != MODE_FAULT))
    {
        log_event(LOG_WARNING, "INPUT",
                  "Unknown mode. Preserving last good mode.");
        input->requested_mode = s_last_good_input.requested_mode;
        all_valid = false;
    }
    else
    {
        s_last_good_input.requested_mode = input->requested_mode;
    }

    input->input_valid = all_valid;

    if (all_valid)
    {
        log_event(LOG_INFO, "INPUT", "All inputs validated OK.");
    }
}

const char *mode_to_string(VehicleMode_t mode)
{
    const char *result;
    switch (mode)
    {
        case MODE_OFF:         result = "OFF";         break;
        case MODE_ACC:         result = "ACC";         break;
        case MODE_IGNITION_ON: result = "IGNITION_ON"; break;
        case MODE_FAULT:       result = "FAULT";       break;
        default:               result = "INVALID";     break;
    }
    return result;
}
