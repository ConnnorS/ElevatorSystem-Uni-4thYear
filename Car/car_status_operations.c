// basic C stuff
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// pthreads
#include <pthread.h>
// my functions
#include "car_helpers.h"

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

void closing_doors(car_shared_mem *shm_status_ptr, __useconds_t *delay_ms)
{
  strcpy(shm_status_ptr->status, "Closing");

  /* constantly try and close doors while there's an obstruction */
  while (shm_status_ptr->door_obstruction)
  {
    // no need to opening_doors() as safety system sets that
    pthread_mutex_unlock(&shm_status_ptr->mutex);
    usleep(*delay_ms * 1000);
    pthread_mutex_lock(&shm_status_ptr->mutex);

    open_doors(shm_status_ptr);

    pthread_mutex_unlock(&shm_status_ptr->mutex);
    usleep(*delay_ms * 1000);
    pthread_mutex_lock(&shm_status_ptr->mutex);

    strcpy(shm_status_ptr->status, "Closing");

    pthread_mutex_unlock(&shm_status_ptr->mutex);
    usleep(*delay_ms * 1000);
    pthread_mutex_lock(&shm_status_ptr->mutex);

    close_doors(shm_status_ptr);
  }

  pthread_cond_broadcast(&shm_status_ptr->cond);
}

void door_open_close(car_shared_mem *shm_status_ptr, __useconds_t *delay_ms)
{
  opening_doors(shm_status_ptr);

  pthread_mutex_unlock(&shm_status_ptr->mutex);
  usleep(*delay_ms * 1000);
  pthread_mutex_lock(&shm_status_ptr->mutex);

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

void handle_service_mode(car_shared_mem *shm_status_ptr)
{
  if (strcmp(shm_status_ptr->status, "Between") == 0)
  {
    close_doors(shm_status_ptr);
  }
  /* change dest floor to curr floor */
  strcpy(shm_status_ptr->destination_floor, shm_status_ptr->current_floor);
}