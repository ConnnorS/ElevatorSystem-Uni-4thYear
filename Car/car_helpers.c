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

void go_to_floor(car_thread_data *data, char *destination)
{
  /* update the destination floor in the shared memory */
  pthread_mutex_lock(&data->ptr->mutex);
  strcpy(data->ptr->destination_floor, destination);
  /* create ints for easier comparison */
  int destination_int = floor_char_to_int(data->ptr->destination_floor);
  int current_int = floor_char_to_int(data->ptr->current_floor);
  printf("Moving from floor %s to %s\n", data->ptr->current_floor, data->ptr->destination_floor);
  pthread_mutex_unlock(&data->ptr->mutex);

  /* set the car to between mode */
  set_between(data);

  /* determine the direction the car will move */
  int direction = 0;
  if (current_int < destination_int)
  {
    direction = 1;
  }
  else
  {
    direction = -1;
  }

  /* now move the floors */
  while (current_int != destination_int)
  {
    /* move the floor up or down by 1 */
    current_int += direction;

    /* update the current floor of the car */
    pthread_mutex_lock(&data->ptr->mutex);
    floor_int_to_char(current_int, data->ptr->current_floor);
    pthread_mutex_unlock(&data->ptr->mutex);

    /* delay until the next step */
    sleep(data->delay_ms / 1000);
  }

  /* open the doors */
  opening_doors(data);
  sleep(data->delay_ms / 1000);
  open_doors(data);
  sleep(data->delay_ms / 1000);
  /* then close the doors */
  closing_doors(data);
  sleep(data->delay_ms / 1000);
  close_doors(data);
}