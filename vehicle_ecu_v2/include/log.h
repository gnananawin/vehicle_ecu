#ifndef LOG_H
#define LOG_H

#include "types.h"
#include <stdio.h>

/* ANSI color codes for console output */
#define LOG_COLOR_RESET   "\033[0m"
#define LOG_COLOR_RED     "\033[1;31m"
#define LOG_COLOR_GREEN   "\033[1;32m"
#define LOG_COLOR_YELLOW  "\033[1;33m"
#define LOG_COLOR_BLUE    "\033[1;34m"
#define LOG_COLOR_MAGENTA "\033[1;35m"
#define LOG_COLOR_CYAN    "\033[1;36m"
#define LOG_COLOR_WHITE   "\033[1;37m"

#define ECU_LOG_FILENAME  "ecu_last_cycle.log"

void log_init(void);
void log_banner(void);
void log_separator(void);
void log_scenario_header(uint32_t scenario_num, const char *description);
void log_event(LogLevel_t level, const char *module, const char *message);
void log_illegal_transition(VehicleMode_t from, VehicleMode_t to);
void log_state_change(SystemState_t from, SystemState_t to, const char *reason);
void log_fault_set(FaultType_t fault, uint32_t counter);
void log_fault_cleared(FaultType_t fault);
void log_fault_persistent(FaultType_t fault, uint32_t cycles);
void log_cycle_summary(const VehicleInput_t  *input,
                       const VehicleStatus_t *status,
                       const FaultStatus_t   *faults);
void log_file_flush_last_cycle(const VehicleInput_t  *input,
                               const VehicleStatus_t *status,
                               const FaultStatus_t   *faults);

#endif /* LOG_H */
