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

#include "car_helpers.h"

void validate_floor_range(int floor)
{
  if (floor < -99)
  {
    printf("Lowest floor or highest floor cannot be lower than B99\n");
    exit(1);
  }
  else if (floor > 999)
  {
    printf("Lowest floor or highest floor cannot be higher than 999\n");
    exit(1);
  }
  else if (floor == 0)
  {
    printf("Floor cannot be zero\n");
    exit(1);
  }
}

void compare_highest_lowest(int lowest, int highest)
{
  if (highest < lowest)
  {
    printf("Highest floor (%s%d) cannot be lower than the lowest floor (%s%d)\n",
           (highest < 0) ? "B" : "",
           (highest < 0) ? highest * -1 : highest,
           (lowest < 0) ? "B" : "",
           (lowest < 0) ? lowest * -1 : lowest);
    exit(1);
  }
}

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
  pthread_mutex_init(&shm_status_ptr->mutex, NULL);
  pthread_cond_init(&shm_status_ptr->cond, NULL);
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

int floor_char_to_int(char *floor)
{
  if (floor[0] == 'B')
  {
    floor[0] = '-';
  }
  return atoi(floor);
}

void floor_int_to_char(int floor, char *floorChar)
{
  if (floor < 0)
  {
    sprintf(floorChar, "B%d", abs(floor));
  }
  else
  {
    sprintf(floorChar, "%d", floor);
  }
}

void opening_doors(connect_data_t *data)
{
  pthread_mutex_lock(&data->status->mutex);
  strcpy(data->status->status, "Opening");
  pthread_mutex_unlock(&data->status->mutex);
}

void open_doors(connect_data_t *data)
{
  pthread_mutex_lock(&data->status->mutex);
  strcpy(data->status->status, "Open");
  pthread_mutex_unlock(&data->status->mutex);
}

void update_current_floor(connect_data_t *data, int current_floor)
{
  pthread_mutex_lock(&data->status->mutex);
  floor_int_to_char(current_floor, data->status->current_floor);
  pthread_mutex_unlock(&data->status->mutex);
}

void update_destination_floor(connect_data_t *data, char *destination_floor)
{
  pthread_mutex_lock(&data->status->mutex);
  strcpy(data->status->destination_floor, destination_floor);
  pthread_mutex_unlock(&data->status->mutex);
}

void change_floor(connect_data_t *data, char *destination_floor)
{
  update_destination_floor(data, destination_floor);

  int destination_floor_int = floor_char_to_int(destination_floor);
  pthread_mutex_lock(&data->status->mutex);
  int current_floor_int = floor_char_to_int(data->status->current_floor);
  pthread_mutex_unlock(&data->status->mutex);

  printf("Moving floors: %d -> %d\n", current_floor_int, destination_floor_int);

  while (1)
  {
    if (current_floor_int == destination_floor_int)
    {
      opening_doors(data);
      sleep(data->info->delay_ms / 1000);
      open_doors(data);
      return;
    }

    /* Change the floor up or down by 1 */
    /* And if the previous current floor is -1 or 1, we will make the next
    current floor 1 or -1 (skipping over 0)*/
    if (current_floor_int < destination_floor_int)
    {
      if (current_floor_int == -1)
      {
        current_floor_int = 1;
      }
      else
      {
        current_floor_int++;
      }
    }
    else
    {
      if (current_floor_int == 1)
      {
        current_floor_int = -1;
      }
      else
      {
        current_floor_int--;
      }
    }

    /* now update the current floor */
    update_current_floor(data, current_floor_int);

    /* And now delay until the next step */
    sleep(data->info->delay_ms / 1000);
  }
}