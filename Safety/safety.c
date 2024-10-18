#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "safety_helpers.h"
#include "../type_conversions.h"

/* global variables */
car_shared_mem *shm_status_ptr;
int shm_status_fd;

#include <string.h>

int data_consistent()
{
  /* ignore if emergency mode active */
  if (shm_status_ptr->emergency_mode == 1)
  {
    return 1;
  }

  /* validate status */
  const char *valid_status[] = {"Open", "Opening", "Closed", "Closing", "Between"};
  int status_is_valid = 0;
  for (int i = 0; i < sizeof(valid_status) / sizeof(valid_status[0]); i++)
  {
    if (strcmp(shm_status_ptr->status, valid_status[i]) == 0)
    {
      status_is_valid = 1;
      break;
    }
  }

  /* validate floor nums */
  int valid_floors = 1;
  // validate size
  if (!check_floor_length(shm_status_ptr->current_floor) || !check_floor_length(shm_status_ptr->destination_floor))
  {
    valid_floors = 0;
  }
  // validate contents
  if (!check_floor_contents(shm_status_ptr->current_floor) || !check_floor_contents(shm_status_ptr->destination_floor))
  {
    valid_floors = 0;
  }

  /* validate fields */
  int fields_are_valid = 1;
  if (shm_status_ptr->open_button < 0 || shm_status_ptr->open_button > 1 ||
      shm_status_ptr->close_button < 0 || shm_status_ptr->close_button > 1 ||
      shm_status_ptr->door_obstruction < 0 || shm_status_ptr->door_obstruction > 1 ||
      shm_status_ptr->overload < 0 || shm_status_ptr->overload > 1 ||
      shm_status_ptr->emergency_stop < 0 || shm_status_ptr->emergency_stop > 1 ||
      shm_status_ptr->individual_service_mode < 0 || shm_status_ptr->individual_service_mode > 1 ||
      shm_status_ptr->emergency_mode < 0 || shm_status_ptr->emergency_mode > 1)
  {
    fields_are_valid = 0;
  }

  /* validate door obstruction */
  int door_obs_valid = 1;
  if (shm_status_ptr->door_obstruction == 1 &&
      strcmp(shm_status_ptr->status, "Opening") != 0 &&
      strcmp(shm_status_ptr->status, "Closing") != 0)
  {
    door_obs_valid = 0;
  }

  return status_is_valid && fields_are_valid && door_obs_valid && valid_floors;
}

void exit_cleanup(int signal)
{
  printf("Received exit code %d\n", signal);
  printf("Cleaning up...\n");

  if (munmap(shm_status_ptr, sizeof(car_shared_mem)) == -1)
  {
    printf("munmap failed\n");
  }

  close(shm_status_fd);

  printf("Cleanup completed\n");
  exit(0);
}

int main(int argc, char **argv)
{
  /* validate user input */
  if (argc != 2)
  {
    printf("Usage: {car name}\n");
    return 1;
  }

  /* confirm the safety system and name */
  char shm_status_name[64];
  snprintf(shm_status_name, sizeof(shm_status_name), "/car%s", argv[1]);

  signal(SIGINT, exit_cleanup); // Handle Ctrl+C

  /* open the shared memory object */
  shm_status_fd = shm_open(shm_status_name, O_RDWR, 0666);
  if (shm_status_fd == -1)
  {
    printf("Unable to access car %s.\n", argv[1]);
    return 1;
  }

  /* map the shared memory */
  shm_status_ptr = mmap(0, sizeof(car_shared_mem), PROT_WRITE, MAP_SHARED, shm_status_fd, 0);
  if (shm_status_ptr == MAP_FAILED)
  {
    perror("mmap failed");
    close(shm_status_fd);
    exit(1);
  }

  /* constantly check for updates */
  while (1)
  {
    pthread_mutex_lock(&shm_status_ptr->mutex);

    /* wait while ... */
    while (shm_status_ptr->door_obstruction == 0 && // there is no door obstruction
           shm_status_ptr->emergency_stop == 0 &&   // emergency stop hasn't been pressed
           shm_status_ptr->overload == 0 &&         // the overload sensor hasn't been tripped
           data_consistent()                        // the car's status is valid
    )
    {
      pthread_cond_wait(&shm_status_ptr->cond, &shm_status_ptr->mutex);
    }

    /* obstruction while doors closing */
    if (shm_status_ptr->door_obstruction == 1 && strcmp(shm_status_ptr->status, "Closing") == 0)
    {
      strcpy(shm_status_ptr->status, "Opening\n");
    }
    /* emergency stop */
    else if (shm_status_ptr->emergency_stop == 1 && shm_status_ptr->emergency_mode == 0)
    {
      printf("The emergency stop button has been pressed!\n");
      shm_status_ptr->emergency_mode = 1;
    }
    /* overload */
    else if (shm_status_ptr->overload == 1 && shm_status_ptr->emergency_mode == 0)
    {
      printf("The overload sensor has been tripped!\n");
      shm_status_ptr->emergency_mode = 1;
    }
    /* data error */
    else if (!data_consistent())
    {
      printf("Data consistency error!\n");
      shm_status_ptr->emergency_mode = 1;
    }

    pthread_mutex_unlock(&shm_status_ptr->mutex);
    pthread_cond_broadcast(&shm_status_ptr->cond);
  }

  exit_cleanup(1); // Cleanup at the end (though this will never be reached)
  return 0;
}