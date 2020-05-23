#define FITNESS_DEVICES_FILE "fitness_devices.dat"
#define FITNESS_DEVICES_COUNT 5
#define MAX_TIME_FOR_REP 180
#define MAX_REPS_FOR_DEVICE 5

typedef enum FitnessDeviceType {
    TREADMILL,
    GLADIATOR,
    BENCH_PRESS,
    HIP_PRESS,
    BICYCLE,
} fitness_device_type;

typedef struct FitnessDevice {
    fitness_device_type type;
    int is_currently_used;
    int id;
} s_fitness_device;

typedef struct FitnessDeviceNode {
    s_fitness_device device;
    struct FitnessDeviceNode *next;
} n_fitness_device;