// basic C stuff
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// pthreads
#include <pthread.h>
// my functions
#include "car_helpers.h"

void opening_doors(car_shared_mem *shm_status_ptr)
{
  strcpy(shm_status_ptr->status, "Opening");
  printf("Opening doors\n");
}

void open_doors(car_shared_mem *shm_status_ptr)
{
  strcpy(shm_status_ptr->status, "Open");
  printf("Doors are open\n");
}

void closing_doors(car_shared_mem *shm_status_ptr)
{
  strcpy(shm_status_ptr->status, "Closing");
  printf("Closing doors\n");
}

void close_doors(car_shared_mem *shm_status_ptr)
{
  strcpy(shm_status_ptr->status, "Closed");
  printf("Doors are closed\n");
}

void set_between(car_shared_mem *shm_status_ptr)
{
  strcpy(shm_status_ptr->status, "Between");
}

void get_status(car_shared_mem *shm_status_ptr, char *status)
{
  strcpy(status, shm_status_ptr->status);
}