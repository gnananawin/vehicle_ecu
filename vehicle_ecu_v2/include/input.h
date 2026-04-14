#ifndef INPUT_H
#define INPUT_H

#include "types.h"

void input_init(VehicleInput_t *input);
void input_set_mode(InputMode_t mode);
void input_load_scenarios(const TestScenario_t *scenarios, uint32_t count);
bool input_scenarios_done(void);
void read_inputs(VehicleInput_t *input);
void validate_inputs(VehicleInput_t *input, VehicleStatus_t *status);
const char *mode_to_string(VehicleMode_t mode);

#endif /* INPUT_H */
