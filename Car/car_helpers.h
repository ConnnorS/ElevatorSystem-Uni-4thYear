#include <stdint.h>

#define CAR_NAME_LENGTH 32

typedef struct
{
  pthread_mutex_t mutex;           // Locked while accessing struct contents
  pthread_cond_t cond;             // Signalled when the contents change
  char current_floor[4];           // C string in the range B99-B1 and 1-999
  char destination_floor[4];       // Same format as above
  char status[8];                  // C string indicating the elevator's status
  uint8_t open_button;             // 1 if open doors button is pressed, else 0
  uint8_t close_button;            // 1 if close doors button is pressed, else 0
  uint8_t door_obstruction;        // 1 if obstruction detected, else 0
  uint8_t overload;                // 1 if overload detected
  uint8_t emergency_stop;          // 1 if stop button has been pressed, else 0
  uint8_t individual_service_mode; // 1 if in individual service mode, else 0
  uint8_t emergency_mode;          // 1 if in emergency mode, else 0
} car_shared_mem;

typedef struct
{
  char name[CAR_NAME_LENGTH];
  char lowest_floor[4];
  char highest_floor[4];
  int delay_ms;
} car_info;

typedef struct connect_data
{
  int socketFd;
  car_info *info;
  car_shared_mem *status;
} connect_data_t;

void validate_floor_range(int floor);
void compare_highest_lowest(int lowest, int highest);
int do_shm_open(char *shm_status_name);
void do_ftruncate(int fd, int size);
void add_default_values(car_shared_mem *shm_status_ptr, const char *lowest_floor_char);
void change_floor(connect_data_t *data, char *floor);