// basic C stuff
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// pthreads
#include <pthread.h>
// my functions
#include "car_helpers.h"

void opening_doors(connect_data_t *data)
{
  pthread_mutex_lock(&data->status->mutex);
  strcpy(data->status->status, "Opening");
  printf("Opening doors\n");
  pthread_mutex_unlock(&data->status->mutex);
}

void open_doors(connect_data_t *data)
{
  pthread_mutex_lock(&data->status->mutex);
  strcpy(data->status->status, "Open");
  printf("Doors are open\n");
  pthread_mutex_unlock(&data->status->mutex);
}

void closing_doors(connect_data_t *data)
{
  pthread_mutex_lock(&data->status->mutex);
  strcpy(data->status->status, "Closing");
  printf("Closing doors\n");
  pthread_mutex_unlock(&data->status->mutex);
}

void close_doors(connect_data_t *data)
{
  pthread_mutex_lock(&data->status->mutex);
  strcpy(data->status->status, "Closed");
  printf("Doors are closed\n");
  pthread_mutex_unlock(&data->status->mutex);
}

void set_between(connect_data_t *data)
{
  pthread_mutex_lock(&data->status->mutex);
  strcpy(data->status->status, "Between");
  printf("Car is between floors\n");
  pthread_mutex_unlock(&data->status->mutex);
}

void get_status(connect_data_t *data, char *status)
{

  pthread_mutex_lock(&data->status->mutex);
  strcpy(status, data->status->status);
  pthread_mutex_unlock(&data->status->mutex);
}