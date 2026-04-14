#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* Software version */
#define ECU_SW_VERSION_MAJOR  1U
#define ECU_SW_VERSION_MINOR  3U
#define ECU_SW_VERSION_PATCH  0U

#define ECU_UNUSED(x)  ((void)(x))

/* Vehicle sensor limits */
#define SPEED_MIN_KPH         0
#define SPEED_MAX_KPH         200
#define SPEED_OVERSPEED_KPH   120

#define TEMP_MIN_CELSIUS      (-40)
#define TEMP_MAX_CELSIUS      150
#define TEMP_HIGH_CELSIUS     95
#define TEMP_CRITICAL_CELSIUS 110

#define GEAR_MIN              0U
#define GEAR_MAX              5U

/* Fault persistence: 3 consecutive cycles -> persistent, 1 persistent -> SAFE */
#define FAULT_PERSISTENT_THRESHOLD  3U
#define FAULT_SAFE_STATE_THRESHOLD  1U

/* Buffer size for last-cycle file log */
#define CYCLE_LOG_BUFFER_SIZE  4096U

/* Mode encoding: 0=OFF, 1=ACC (engine off, gear ok), 2=IGNITION_ON, 3=FAULT */
typedef enum
{
    MODE_OFF         = 0,
    MODE_ACC         = 1,
    MODE_IGNITION_ON = 2,
    MODE_FAULT       = 3
} VehicleMode_t;

typedef enum
{
    TEMP_STATUS_NORMAL   = 0,
    TEMP_STATUS_HIGH     = 1,
    TEMP_STATUS_CRITICAL = 2
} TempStatus_t;

typedef enum
{
    STATE_NORMAL   = 0,
    STATE_DEGRADED = 1,
    STATE_SAFE     = 2
} SystemState_t;

/* Fault types ordered by priority: index 0 = highest priority */
typedef enum
{
    FAULT_CRITICAL_OVERHEAT = 0,
    FAULT_INVALID_GEAR      = 1,
    FAULT_OVERSPEED         = 2,
    FAULT_INVALID_MODE      = 3,
    FAULT_SPEED_IN_OFF      = 4,
    FAULT_SPEED_IN_ACC      = 5,
    FAULT_COUNT             = 6
} FaultType_t;

#define FAULT_BIT_CRITICAL_OVERHEAT  (1U << 0U)
#define FAULT_BIT_INVALID_GEAR       (1U << 1U)
#define FAULT_BIT_OVERSPEED          (1U << 2U)
#define FAULT_BIT_INVALID_MODE       (1U << 3U)
#define FAULT_BIT_SPEED_IN_OFF       (1U << 4U)
#define FAULT_BIT_SPEED_IN_ACC       (1U << 5U)

#define FAULT_SET(flags, mask)       ((flags) |= (mask))
#define FAULT_CLEAR(flags, mask)     ((flags) &= (uint8_t)(~(mask)))
#define FAULT_IS_SET(flags, mask)    (((flags) & (mask)) != 0U)

typedef enum
{
    LOG_INFO     = 0,
    LOG_WARNING  = 1,
    LOG_ERROR    = 2,
    LOG_FAULT    = 3,
    LOG_CRITICAL = 4
} LogLevel_t;

typedef enum
{
    INPUT_MODE_INTERACTIVE = 0,
    INPUT_MODE_SCENARIO    = 1
} InputMode_t;

typedef struct
{
    int32_t        speed_kph;
    int32_t        temperature_c;
    uint8_t        gear;
    VehicleMode_t  requested_mode;
    bool           input_valid;
} VehicleInput_t;

typedef struct
{
    VehicleMode_t  current_mode;
    VehicleMode_t  previous_mode;
    SystemState_t  system_state;
    SystemState_t  previous_state;
    TempStatus_t   temp_status;
    bool           overspeed;
    bool           gear_valid;
    bool           speed_in_off;
    bool           speed_in_acc;
    bool           fault_mode_latch;
    uint32_t       cycle_count;
} VehicleStatus_t;

typedef struct
{
    uint8_t     active_flags;
    uint32_t    counters[FAULT_COUNT];
    uint32_t    persistent_counts[FAULT_COUNT];
    bool        is_persistent[FAULT_COUNT];
    uint8_t     persistent_fault_count;
    FaultType_t primary_fault;
    bool        has_primary_fault;
} FaultStatus_t;

typedef struct
{
    int32_t        speed_kph;
    int32_t        temperature_c;
    uint8_t        gear;
    VehicleMode_t  requested_mode;
    const char    *description;
} TestScenario_t;

/* Fixed-size buffer for plain-text file log (no heap) */
typedef struct
{
    char     data[CYCLE_LOG_BUFFER_SIZE];
    uint32_t length;
    uint32_t cycle_num;
} CycleLogBuffer_t;

#endif /* TYPES_H */
