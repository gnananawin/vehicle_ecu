#ifndef FAULT_H
#define FAULT_H

#include "types.h"

extern const uint8_t FAULT_BIT_MASK[FAULT_COUNT];

void        fault_init(FaultStatus_t *faults);
void        set_fault(FaultStatus_t *faults, FaultType_t fault);
void        clear_fault(FaultStatus_t *faults, FaultType_t fault);
void        increment_fault_counter(FaultStatus_t *faults, FaultType_t fault);
void        update_fault_status(FaultStatus_t *faults);
bool        fault_is_active(const FaultStatus_t *faults, FaultType_t fault);
bool        fault_is_persistent(const FaultStatus_t *faults, FaultType_t fault);
void        fault_reset_all(FaultStatus_t *faults);
const char *fault_name(FaultType_t fault);

#endif /* FAULT_H */
