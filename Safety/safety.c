#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include "safety_helpers.h"

/* global variables */
car_shared_mem *shm_status_ptr;
int shm_status_fd;

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
    exit(1);
  }

  /* confirm the safety system and name */
  char shm_status_name[64];
  snprintf(shm_status_name, sizeof(shm_status_name), "/car%s", argv[1]);
  printf("Safety system initialized for %s\n", shm_status_name);

  signal(SIGINT, exit_cleanup); // Handle Ctrl+C

  /* open the shared memory object */
  shm_status_fd = shm_open(shm_status_name, O_RDWR, 0666);
  if (shm_status_fd == -1)
  {
    perror("Unable to open shared memory object");
    exit(1);
  }
  printf("shm open success\n");

  /* map the shared memory */
  shm_status_ptr = mmap(0, sizeof(car_shared_mem), PROT_WRITE, MAP_SHARED, shm_status_fd, 0);
  if (shm_status_ptr == MAP_FAILED)
  {
    perror("mmap failed");
    close(shm_status_fd);
    exit(1);
  }
  printf("mmap success\n");

  /* constantly check for updates */
  while (1)
  {
    pthread_mutex_lock(&shm_status_ptr->mutex);
    printf("Obstruction: %d\n", shm_status_ptr->door_obstruction);
    printf("Emergency stop: %d\n", shm_status_ptr->emergency_stop);
    printf("Emergency mode: %d\n", shm_status_ptr->emergency_mode);

    while (shm_status_ptr->door_obstruction == 0 && shm_status_ptr->emergency_stop == 0 && shm_status_ptr->overload == 0)
    {
      pthread_cond_wait(&shm_status_ptr->cond, &shm_status_ptr->mutex);
    }

    printf("Safety system engaged!\n");

    /* obstruction while doors opening */
    if (shm_status_ptr->door_obstruction == 1 && strcmp(shm_status_ptr->status, "CLOSING") == 0)
    {
      printf("Door obstructed while closing\n");
      printf("Opening doors...\n");
      strcpy(shm_status_ptr->status, "OPENING");
      shm_status_ptr->door_obstruction = 0;
    }
    /* emergency stop */
    else if (shm_status_ptr->emergency_stop == 1 && shm_status_ptr->emergency_mode == 0)
    {
      printf("The emergency stop button has been pressed!\n");
      printf("Entering emergency mode\n");
      shm_status_ptr->emergency_mode = 1;
      shm_status_ptr->emergency_stop = 0;
    }
    /* overload */
    else if (shm_status_ptr->overload == 1 && shm_status_ptr->emergency_mode == 0)
    {
      printf("The overload sensor has been tripped!\n");
      printf("Entering emergency mode\n");
      shm_status_ptr->emergency_mode = 1;
      shm_status_ptr->overload = 0;
    }
    /* data error */
    /* [add in here]*/

    pthread_mutex_unlock(&shm_status_ptr->mutex);
    pthread_cond_broadcast(&shm_status_ptr->cond);
  }

  exit_cleanup(1); // Cleanup at the end (though this will never be reached)
  return 0;
}