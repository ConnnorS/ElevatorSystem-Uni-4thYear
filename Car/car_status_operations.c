// basic C stuff
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// pthreads
#include <pthread.h>
// my functions
#include "car_helpers.h"

void opening_doors(car_thread_data *data)
{
  pthread_mutex_lock(&data->ptr->mutex);
  strcpy(data->ptr->status, "Opening");
  printf("Opening doors\n");
  pthread_mutex_unlock(&data->ptr->mutex);
}

void open_doors(car_thread_data *data)
{
  pthread_mutex_lock(&data->ptr->mutex);
  strcpy(data->ptr->status, "Open");
  printf("Doors are open\n");
  pthread_mutex_unlock(&data->ptr->mutex);
}

void closing_doors(car_thread_data *data)
{
  pthread_mutex_lock(&data->ptr->mutex);
  strcpy(data->ptr->status, "Closing");
  printf("Closing doors\n");
  pthread_mutex_unlock(&data->ptr->mutex);
}

void close_doors(car_thread_data *data)
{
  pthread_mutex_lock(&data->ptr->mutex);
  strcpy(data->ptr->status, "Closed");
  printf("Doors are closed\n");
  pthread_mutex_unlock(&data->ptr->mutex);
}

void set_between(car_thread_data *data)
{
  pthread_mutex_lock(&data->ptr->mutex);
  strcpy(data->ptr->status, "Between");
  printf("Car is between floors\n");
  pthread_mutex_unlock(&data->ptr->mutex);
}

void get_status(car_thread_data *data, char *status)
{

  pthread_mutex_lock(&data->ptr->mutex);
  strcpy(status, data->ptr->status);
  pthread_mutex_unlock(&data->ptr->mutex);
}