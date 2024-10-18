// basic C stuff
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// threads
#include <pthread.h>
// shared memory
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
// networks
#include <arpa/inet.h>
// my headers
#include "car_helpers.h"
#include "car_status_operations.h"
#include "../type_conversions.h"
// comms
#include "../common_comms.h"

int do_shm_open(char *shm_status_name)
{
  // create the shared memory object
  shm_unlink(shm_status_name); // unlink any old objects just in case
  int fd = shm_open(shm_status_name, O_CREAT | O_RDWR, 0666);
  if (fd == -1)
  {
    perror("shm_open()");
    exit(1);
  }

  return fd;
}

void do_ftruncate(int fd, int size)
{
  // set the size of the shared memory object
  int ftruncate_success = ftruncate(fd, size);
  if (ftruncate_success == -1)
  {
    perror("ftruncate()");
    exit(1);
  }
}

void init_shared_mem(car_shared_mem *shm_status_ptr, const char *lowest_floor_char)
{
  // initialise the shared mutex
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  if (pthread_mutex_init(&shm_status_ptr->mutex, &attr) != 0)
  {
    printf("Mutex init failed\n");
  }
  // initialise the shared condition variable
  pthread_condattr_t cond_attr;
  pthread_condattr_init(&cond_attr);
  pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
  if (pthread_cond_init(&shm_status_ptr->cond, &cond_attr) != 0)
  {
    printf("Cond init failed\n");
  }

  strcpy(shm_status_ptr->current_floor, lowest_floor_char);
  strcpy(shm_status_ptr->destination_floor, lowest_floor_char);
  strcpy(shm_status_ptr->status, "Closed");
  shm_status_ptr->open_button = 0;
  shm_status_ptr->close_button = 0;
  shm_status_ptr->door_obstruction = 0;
  shm_status_ptr->overload = 0;
  shm_status_ptr->emergency_stop = 0;
  shm_status_ptr->individual_service_mode = 0;
  shm_status_ptr->emergency_mode = 0;
}

void move_floors(car_shared_mem *shm_status_ptr, int direction, int *delay_ms)
{
  /* if the doors are open, close them */
  if (strcmp(shm_status_ptr->status, "Open") == 0)
  {
    closing_doors(shm_status_ptr, delay_ms);

    pthread_mutex_unlock(&shm_status_ptr->mutex);
    usleep(*delay_ms * 1000);
    pthread_mutex_lock(&shm_status_ptr->mutex);

    close_doors(shm_status_ptr);
  }

  /* now set the car to between */
  set_between(shm_status_ptr);

  pthread_mutex_unlock(&shm_status_ptr->mutex);
  usleep(*delay_ms * 1000);
  pthread_mutex_lock(&shm_status_ptr->mutex);

  /* now move up or down one floor */
  int current_floor_int = floor_char_to_int(shm_status_ptr->current_floor);
  current_floor_int += direction;
  if (current_floor_int == 0) // avoid going to floor 0
  {
    current_floor_int += direction;
  }
  floor_int_to_char(current_floor_int, shm_status_ptr->current_floor);
  pthread_cond_broadcast(&shm_status_ptr->cond);

  /* if the car hits the destination floor - open and close the doors only if not in service mode */
  if (strcmp(shm_status_ptr->current_floor, shm_status_ptr->destination_floor) == 0 && shm_status_ptr->individual_service_mode == 0)
  {
    door_open_close(shm_status_ptr, delay_ms, shm_status_ptr->door_obstruction);
  }
}

void handle_dest_floor_change(car_shared_mem *shm_status_ptr, int *delay_ms, int *lowest_floor_int, int *highest_floor_int)
{
  /* find the direction the car must move */
  int direction = 0;
  int current_floor = floor_char_to_int(shm_status_ptr->current_floor);
  int destination_floor = floor_char_to_int(shm_status_ptr->destination_floor);

  if (current_floor < destination_floor)
  {
    direction = 1; // up
  }
  else if (current_floor > destination_floor)
  {
    direction = -1; // down
  }

  /* ensure the car won't go out of bounds */
  if (current_floor + direction < *lowest_floor_int || current_floor + direction > *highest_floor_int)
  {
    strcpy(shm_status_ptr->destination_floor, shm_status_ptr->current_floor);
    return;
  }

  /* if the car has been sent the floor it's on */
  if (direction == 0)
  {
    door_open_close(shm_status_ptr, delay_ms, shm_status_ptr->door_obstruction);
    return;
  }

  /* now move floors */
  move_floors(shm_status_ptr, direction, delay_ms);
}
