#ifndef STATE_H
#define STATE_H

#include "types.h"

void        state_init(VehicleStatus_t *status);
void        evaluate_system_state(VehicleStatus_t     *status,
                                  const FaultStatus_t *faults);
void        state_reset(VehicleStatus_t *status, FaultStatus_t *faults);
const char *state_to_string(SystemState_t state);

#endif /* STATE_H */
