#include <pthread.h>
#include <stdint.h>

/* struct definitions */
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
  int fd;       // file descriptor of the connected server
  __useconds_t delay_ms; // the delay of the car
  char lowest_floor[4];
  char highest_floor[4];
} car_thread_data;

/* functions used by the car component */
int do_shm_open(char *shm_status_name);
void do_ftruncate(int fd, int size);
void init_shared_mem(car_shared_mem *shm_status_ptr, const char *lowest_floor_char);
void move_floors(car_shared_mem *shm_status_ptr, int direction, __useconds_t *delay_ms);
void handle_dest_floor_change(car_shared_mem *shm_status_ptr, __useconds_t *delay_ms, int *lowest_floor_int, int *highest_floor_int);