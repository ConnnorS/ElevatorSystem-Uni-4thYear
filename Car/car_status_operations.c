// basic C stuff
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// pthreads
#include <pthread.h>
// my functions
#include "car_helpers.h"

void handle_obstruction(car_shared_mem *shm_status_ptr, int *delay_ms);

void opening_doors(car_shared_mem *shm_status_ptr)
{
  strcpy(shm_status_ptr->status, "Opening");
  pthread_cond_broadcast(&shm_status_ptr->cond);
}

void open_doors(car_shared_mem *shm_status_ptr)
{
  strcpy(shm_status_ptr->status, "Open");
  pthread_cond_broadcast(&shm_status_ptr->cond);
}

void closing_doors(car_shared_mem *shm_status_ptr)
{
  strcpy(shm_status_ptr->status, "Closing");
  pthread_cond_broadcast(&shm_status_ptr->cond);
}

void close_doors(car_shared_mem *shm_status_ptr, int *delay_ms)
{
  // check for obstruction
  while (shm_status_ptr->door_obstruction)
  {
    handle_obstruction(shm_status_ptr, delay_ms);
  }

  strcpy(shm_status_ptr->status, "Closed");
  pthread_cond_broadcast(&shm_status_ptr->cond);
}

void set_between(car_shared_mem *shm_status_ptr)
{
  strcpy(shm_status_ptr->status, "Between");
  pthread_cond_broadcast(&shm_status_ptr->cond);
}

void get_status(car_shared_mem *shm_status_ptr, char *status)
{
  strcpy(status, shm_status_ptr->status);
}

void handle_obstruction(car_shared_mem *shm_status_ptr, int *delay_ms)
{
  opening_doors(shm_status_ptr);

  pthread_mutex_unlock(&shm_status_ptr->mutex);
  usleep(*delay_ms * 1000);
  pthread_mutex_lock(&shm_status_ptr->mutex);

  open_doors(shm_status_ptr);

  pthread_mutex_unlock(&shm_status_ptr->mutex);
  usleep(*delay_ms * 1000);
  pthread_mutex_lock(&shm_status_ptr->mutex);

  closing_doors(shm_status_ptr);

  pthread_mutex_unlock(&shm_status_ptr->mutex);
  usleep(*delay_ms * 1000);
  pthread_mutex_lock(&shm_status_ptr->mutex);
}