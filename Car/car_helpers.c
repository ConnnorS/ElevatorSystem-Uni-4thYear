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

void add_default_values(car_shared_mem *shm_status_ptr, const char *lowest_floor_char)
{
  // initialise the shared mutex
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&shm_status_ptr->mutex, &attr);
  // initialise the shared condition variable
  pthread_condattr_t cond_attr;
  pthread_condattr_init(&cond_attr);
  pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(&shm_status_ptr->cond, &cond_attr);

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

void move_floors(car_shared_mem *shm_status_ptr, int direction, int delay_ms)
{
  /* now set the car to between */
  set_between(shm_status_ptr);

  /* now move up or down one floor */
  int current_floor_int = floor_char_to_int(shm_status_ptr->current_floor);
  current_floor_int += direction;
  floor_int_to_char(current_floor_int, shm_status_ptr->current_floor);

  printf("Car is now on floor %s\n", shm_status_ptr->current_floor);

  /* if the car hits the destination floor */
  if (strcmp(shm_status_ptr->current_floor, shm_status_ptr->destination_floor) == 0)
  {
    opening_doors(shm_status_ptr);

    pthread_mutex_unlock(&shm_status_ptr->mutex);
    sleep(delay_ms / 1000);
    pthread_mutex_lock(&shm_status_ptr->mutex);

    open_doors(shm_status_ptr);

    pthread_mutex_unlock(&shm_status_ptr->mutex);
    sleep(delay_ms / 1000);
    pthread_mutex_lock(&shm_status_ptr->mutex);

    closing_doors(shm_status_ptr);

    pthread_mutex_unlock(&shm_status_ptr->mutex);
    sleep(delay_ms / 1000);
    pthread_mutex_lock(&shm_status_ptr->mutex);

    close_doors(shm_status_ptr);
  }
}