#ifndef CONTROL_H
#define CONTROL_H

#include "types.h"

void run_control_checks(const VehicleInput_t *input,
                        VehicleStatus_t      *status,
                        FaultStatus_t        *faults);
void check_overspeed(const VehicleInput_t *input,
                     VehicleStatus_t      *status,
                     FaultStatus_t        *faults);
void check_temperature(const VehicleInput_t *input,
                       VehicleStatus_t      *status,
                       FaultStatus_t        *faults);
void check_gear(const VehicleInput_t *input,
                VehicleStatus_t      *status,
                FaultStatus_t        *faults);
void check_speed_in_off(const VehicleInput_t *input,
                        VehicleStatus_t      *status,
                        FaultStatus_t        *faults);
void check_speed_in_acc(const VehicleInput_t *input,
                        VehicleStatus_t      *status,
                        FaultStatus_t        *faults);

#endif /* CONTROL_H */
