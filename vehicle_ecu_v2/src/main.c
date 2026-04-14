#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "input.h"
#include "mode.h"
#include "control.h"
#include "fault.h"
#include "state.h"
#include "log.h"

/*
 * Scheduler execution order (fixed every cycle):
 *   1. read_inputs()          - fetch raw sensor data
 *   2. validate_inputs()      - range-check and sanitize
 *   3. update_mode()          - mode transition / FAULT escalation
 *   4. run_control_checks()   - overspeed / temp / gear evaluation
 *   5. update_fault_status()  - persistence tracking
 *   6. evaluate_system_state()- NORMAL / DEGRADED / SAFE decision
 *   7. log_cycle_summary()    - console + file output
 */

static void init_system(VehicleStatus_t *status, FaultStatus_t *faults)
{
    log_init();

    input_init(&(VehicleInput_t){0});
    mode_init(status);
    fault_init(faults);
    state_init(status);

    status->cycle_count      = 0U;
    status->overspeed        = false;
    status->gear_valid       = true;
    status->temp_status      = TEMP_STATUS_NORMAL;
    status->speed_in_off     = false;
    status->speed_in_acc     = false;
    status->fault_mode_latch = false;

    log_event(LOG_INFO, "SYSTEM", "ECU initialization complete. Scheduler starting.");
    log_separator();
}

static void system_halt(void)
{
    printf("\n");
    printf(LOG_COLOR_RED
           "╔══════════════════════════════════════════════════════════════╗\n"
           "║                  *** SYSTEM HALTED ***                       ║\n"
           "║  Persistent fault threshold reached. STATE = SAFE.           ║\n"
           "║  No further input will be accepted.                          ║\n"
           "║  Restart the program to recover.                             ║\n"
           "╚══════════════════════════════════════════════════════════════╝\n"
           LOG_COLOR_RESET "\n");
}

static bool handle_post_fault_interactive(VehicleStatus_t *status,
                                           FaultStatus_t   *faults)
{
    int32_t choice = 0;
    bool    should_continue = false;

    printf(LOG_COLOR_YELLOW
           "\n[RECOVERY OPTIONS]\n"
           "  System is in SAFE state. No sensor input accepted.\n"
           "  Choose your action (as if clicking a mode button):\n"
           "  [1] Manual Reset -> System restarts from OFF mode\n"
           "  [0] Exit         -> Terminate program\n"
           LOG_COLOR_RESET);

    printf(LOG_COLOR_YELLOW
           "  Your choice (1=Manual Reset / 0=Exit): "
           LOG_COLOR_RESET);

    if ((scanf("%d", &choice) == 1) && (choice == 1))
    {
        printf(LOG_COLOR_GREEN
               "\n[RECOVERY] Manual reset selected.\n"
               "  -> Clearing all faults\n"
               "  -> Resetting mode to OFF\n"
               "  -> State returning to NORMAL\n"
               LOG_COLOR_RESET);

        state_reset(status, faults);
        log_event(LOG_INFO, "SYSTEM",
                  "Manual reset complete. System restarted from OFF.");
        should_continue = true;
    }
    else
    {
        printf(LOG_COLOR_RED
               "\n[SYSTEM] Exit selected. Shutting down ECU.\n"
               LOG_COLOR_RESET);
    }

    return should_continue;
}

/* Test scenarios (24 total) */
static const TestScenario_t g_test_scenarios[] =
{
    /* TC1: Normal Start Sequence */
    { 0,   20,  0, MODE_OFF,         "TC1a: Normal Start - OFF mode"                              },
    { 0,   20,  0, MODE_ACC,         "TC1b: Normal Start - OFF -> ACC"                            },
    { 40,  80,  1, MODE_IGNITION_ON, "TC1c: Normal Start - ACC -> IGNITION_ON (speed=40, temp=80, gear=1)" },

    /* TC2: Overspeed */
    { 130, 85,  4, MODE_IGNITION_ON, "TC2: Overspeed (speed=130 kph, limit=120) [MAJOR]"          },

    /* TC3: High Temperature (warning only) */
    { 60,  100, 3, MODE_IGNITION_ON, "TC3: High Temperature (temp=100 degC) - warning only"       },

    /* TC4: Critical Overheat */
    { 60,  115, 3, MODE_IGNITION_ON, "TC4: Critical Overheat (temp=115 degC) [CRITICAL]"          },

    /* TC5: Invalid Gear -> SAFE state */
    { 60,  80,  9, MODE_IGNITION_ON, "TC5: Invalid Gear (gear=9, max=5) -> SAFE state [MAJOR]"    },

    /* TC6: IGNITION_ON -> ACC */
    { 20,  70,  1, MODE_IGNITION_ON, "TC6a: Setup - IGNITION_ON with normal values"               },
    { 0,   65,  0, MODE_ACC,         "TC6b: IGNITION_ON -> ACC (valid controlled downgrade)"       },
    { 0,   65,  2, MODE_ACC,         "TC6c: ACC mode with gear=2 (gear selector accepted in ACC)"  },

    /* TC7: Illegal Mode Transition */
    { 0,   20,  0, MODE_OFF,         "TC7a: Reset to OFF before illegal transition test"          },
    { 0,   20,  0, MODE_IGNITION_ON, "TC7b: Illegal OFF->IGNITION_ON (forces FAULT mode)"         },

    /* TC8: Direct FAULT mode */
    { 0,   20,  0, MODE_FAULT,       "TC8: Direct FAULT input (mode=3) - system enters FAULT mode" },

    /* TC9: Speed in ACC mode */
    { 200, 20,  5, MODE_ACC,         "TC9: Speed=200 kph in ACC mode - FAULT (engine off)"        },

    /* TC10: Multiple faults */
    { 0,   20,  0, MODE_OFF,         "TC10_setup_a: OFF state before multi-fault"                 },
    { 0,   20,  0, MODE_ACC,         "TC10_setup_b: OFF -> ACC"                                    },
    { 140, 120, 9, MODE_IGNITION_ON, "TC10: Multi-fault: Overspeed+Critical Overheat+Invalid Gear" },

    /* TC11: Persistent Fault (3 consecutive cycles -> SAFE) */
    { 0,   20,  0, MODE_OFF,         "TC11_setup_a: Reset to OFF"                                 },
    { 0,   20,  0, MODE_ACC,         "TC11_setup_b: OFF -> ACC"                                    },
    { 0,   115, 2, MODE_IGNITION_ON, "TC11a: Persistent Critical Overheat - Cycle 1/3"            },
    { 0,   115, 2, MODE_IGNITION_ON, "TC11b: Persistent Critical Overheat - Cycle 2/3"            },
    { 0,   115, 2, MODE_IGNITION_ON, "TC11c: Persistent Critical Overheat - Cycle 3/3 -> SAFE"    },

    /* TC12: Recovery */
    { 0,   20,  0, MODE_OFF,         "TC12_setup_a: Reset to OFF"                                 },
    { 0,   20,  0, MODE_ACC,         "TC12_setup_b: OFF -> ACC"                                    },
    { 80,  75,  2, MODE_IGNITION_ON, "TC12: Recovery - Normal inputs after prior faults"          }
};

#define SCENARIO_COUNT  ((uint32_t)(sizeof(g_test_scenarios) / sizeof(g_test_scenarios[0])))

/* Index of the first scenario in each reset group */
#define RESET_BEFORE_TC8   12U
#define RESET_BEFORE_TC9   13U
#define RESET_BEFORE_TC10  14U
#define RESET_BEFORE_TC11  17U
#define RESET_BEFORE_TC12  22U

static void run_one_cycle(VehicleInput_t  *input,
                          VehicleStatus_t *status,
                          FaultStatus_t   *faults)
{
    status->cycle_count++;
    status->speed_in_acc = false;

    read_inputs(input);
    validate_inputs(input, status);
    update_mode(status, input, faults);
    run_control_checks(input, status, faults);
    update_fault_status(faults);
    evaluate_system_state(status, faults);
    log_cycle_summary(input, status, faults);
}

int main(int argc, char *argv[])
{
    VehicleInput_t  input  = {0};
    VehicleStatus_t status = {0};
    FaultStatus_t   faults = {0};
    bool interactive_mode  = false;

    ECU_UNUSED(argv);

    if (argc > 1)
    {
        interactive_mode = true;
    }

    init_system(&status, &faults);

    if (interactive_mode)
    {
        printf(LOG_COLOR_YELLOW
               "\n[INTERACTIVE MODE] Enter vehicle parameters each cycle.\n"
               "  Mode  [0=OFF  1=ACC  2=IGNITION_ON  3=FAULT]\n"
               "\n  Output log file: ecu_last_cycle.log (last cycle only)\n"
               "\nPress Ctrl+C to exit.\n"
               LOG_COLOR_RESET "\n");

        input_set_mode(INPUT_MODE_INTERACTIVE);

        while (1)
        {
            run_one_cycle(&input, &status, &faults);

            if (status.system_state == STATE_SAFE)
            {
                system_halt();
                if (!handle_post_fault_interactive(&status, &faults))
                {
                    return 0;
                }
            }
        }
    }
    else
    {
        uint32_t i;
        bool     halted = false;

        printf(LOG_COLOR_CYAN
               "\n[SCENARIO MODE] Running %u mandatory test scenarios.\n"
               LOG_COLOR_RESET "\n", SCENARIO_COUNT);

        input_set_mode(INPUT_MODE_SCENARIO);
        input_load_scenarios(g_test_scenarios, SCENARIO_COUNT);

        for (i = 0U; (i < SCENARIO_COUNT) && !halted; i++)
        {
            if (i == RESET_BEFORE_TC8)
            {
                printf(LOG_COLOR_CYAN "\n  [SYSTEM RESET] Before TC8\n" LOG_COLOR_RESET);
                state_reset(&status, &faults);
                status.cycle_count = 0U;
            }
            if (i == RESET_BEFORE_TC9)
            {
                printf(LOG_COLOR_CYAN "\n  [SYSTEM RESET] Before TC9\n" LOG_COLOR_RESET);
                state_reset(&status, &faults);
                status.cycle_count = 0U;
            }
            if (i == RESET_BEFORE_TC10)
            {
                printf(LOG_COLOR_CYAN "\n  [SYSTEM RESET] Before TC10\n" LOG_COLOR_RESET);
                state_reset(&status, &faults);
                status.cycle_count = 0U;
            }
            if (i == RESET_BEFORE_TC11)
            {
                printf(LOG_COLOR_CYAN "\n  [SYSTEM RESET] Before TC11\n" LOG_COLOR_RESET);
                state_reset(&status, &faults);
                status.cycle_count = 0U;
            }
            if (i == RESET_BEFORE_TC12)
            {
                printf(LOG_COLOR_CYAN "\n  [SYSTEM RESET] Before TC12\n" LOG_COLOR_RESET);
                state_reset(&status, &faults);
                status.cycle_count = 0U;
            }

            run_one_cycle(&input, &status, &faults);

            if (status.system_state == STATE_SAFE)
            {
                system_halt();
                halted = true;

                if (i < (SCENARIO_COUNT - 1U))
                {
                    printf(LOG_COLOR_CYAN
                           "\n  [AUTO-RESET] Resetting for remaining test scenarios...\n"
                           LOG_COLOR_RESET);
                    state_reset(&status, &faults);
                    status.cycle_count = 0U;
                    halted = false;
                }
            }
        }

        printf("\n\n");
        printf(LOG_COLOR_WHITE
               "╔══════════════════════════════════════════════════════════════╗\n"
               "║                    TEST EXECUTION COMPLETE                  ║\n"
               "╠══════════════════════════════════════════════════════════════╣\n"
               "║  All mandatory test scenarios executed.                      ║\n"
               "║  Review cycle logs above for pass/fail assessment.           ║\n"
               "║  Last cycle plain-text log: ecu_last_cycle.log               ║\n"
               "║  Run with '-i' for interactive (live input) mode.            ║\n"
               "╚══════════════════════════════════════════════════════════════╝\n"
               LOG_COLOR_RESET "\n");
    }

    return 0;
}
