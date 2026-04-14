#ifndef MODE_H
#define MODE_H

#include "types.h"

void mode_init(VehicleStatus_t *status);
bool mode_transition_allowed(VehicleMode_t from, VehicleMode_t to);
void update_mode(VehicleStatus_t      *status,
                 const VehicleInput_t *input,
                 FaultStatus_t        *faults);
void mode_force_fault(VehicleStatus_t *status,
                      FaultStatus_t   *faults,
                      const char      *reason);

#endif /* MODE_H */
