// basic C stuff
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// pthreads
#include <pthread.h>
// my functions
#include "car_helpers.h"

void door_open_close(car_shared_mem *shm_status_ptr, int *delay_ms, int obstruction);

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

void closing_doors(car_shared_mem *shm_status_ptr, int *delay_ms)
{
  // if there's a door obstruction - need to open and close the doors again
  if (shm_status_ptr->door_obstruction)
  {
    // clear obstruction and attempt to open and close the doors again
    shm_status_ptr->door_obstruction = 0;
    door_open_close(shm_status_ptr, delay_ms, shm_status_ptr->door_obstruction);
    return;
  }
  strcpy(shm_status_ptr->status, "Closing");
  pthread_cond_broadcast(&shm_status_ptr->cond);
}

void close_doors(car_shared_mem *shm_status_ptr)
{
  strcpy(shm_status_ptr->status, "Closed");
  pthread_cond_broadcast(&shm_status_ptr->cond);
}

void set_between(car_shared_mem *shm_status_ptr)
{
  strcpy(shm_status_ptr->status, "Between");
  pthread_cond_broadcast(&shm_status_ptr->cond);
}

void door_open_close(car_shared_mem *shm_status_ptr, int *delay_ms, int obstruction)
{
  if (!obstruction) // doors will already be set to opening by the safety system if obstruction
  {
    opening_doors(shm_status_ptr);

    pthread_mutex_unlock(&shm_status_ptr->mutex);
    usleep(*delay_ms * 1000);
    pthread_mutex_lock(&shm_status_ptr->mutex);
  }

  open_doors(shm_status_ptr);

  pthread_mutex_unlock(&shm_status_ptr->mutex);
  usleep(*delay_ms * 1000);
  pthread_mutex_lock(&shm_status_ptr->mutex);

  closing_doors(shm_status_ptr, delay_ms);

  pthread_mutex_unlock(&shm_status_ptr->mutex);
  usleep(*delay_ms * 1000);
  pthread_mutex_lock(&shm_status_ptr->mutex);

  close_doors(shm_status_ptr);

  pthread_mutex_unlock(&shm_status_ptr->mutex);
  usleep(*delay_ms * 1000);
  pthread_mutex_lock(&shm_status_ptr->mutex);
}