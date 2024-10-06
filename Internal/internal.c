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
// my functions
#include "internal_helpers.h"
#include "../type_conversions.h"

/* {file} {car name} {operation} */
int main(int argc, char **argv)
{
  /* validate user input */
  if (argc != 3)
  {
    printf("Useage %s {car name} {operation}\n", argv[0]);
    exit(1);
  }

  char shm_status_name[64] = "/car";
  char operation[8];
  snprintf(shm_status_name, sizeof(shm_status_name), "/car%s", argv[1]);
  strcpy((char *)operation, argv[2]);

  if (verify_operation((char *)operation) == 0)
  {
    printf("Invalid operation specified\nPlease enter \"open\", \"close\", \"stop\", \"service_on\", \"service_off\", \"up\", or \"down\"\n");
    exit(1);
  }

  /* open the shared memory object */
  int shm_status_fd = shm_open(shm_status_name, O_RDWR, 0666);
  if (shm_status_fd == -1)
  {
    printf("Unable to access %s\n", shm_status_name);
    exit(1);
  }

  /* mmap the shared memory */
  car_shared_mem *shm_status_ptr = mmap(NULL, sizeof(car_shared_mem), PROT_READ | PROT_WRITE, MAP_SHARED, shm_status_fd, 0);
  if (shm_status_ptr == MAP_FAILED)
  {
    perror("mmap failed");
    close(shm_status_fd);
    exit(1);
  }

  /* verify the operations */
  pthread_mutex_lock(&shm_status_ptr->mutex);

  /* if the user presses up or down but the car isn't in service mode */
  if ((strcmp(operation, "up") == 0 || strcmp(operation, "down") == 0) && shm_status_ptr->individual_service_mode != 1)
  {
    printf("Operation only allowed in service mode.\n");
    exit(1);
  }

  /* if the up or down command is entered by the doors aren't closed */
  if ((strcmp(operation, "up") == 0 || strcmp(operation, "down") == 0) && strcmp(shm_status_ptr->status, "Closed") != 0)
  {
    printf("Operation not allowed while doors are open.\n");
    exit(1);
  }

  /* if the up or down command is entered but the car is moving */
  if ((strcmp(operation, "up") == 0 || strcmp(operation, "down") == 0) && strcmp(shm_status_ptr->status, "Between") != 0)
  {
    printf("Operation not allowed while elevator is moving.\n");
    exit(1);
  }

  /* check which operation was entered and take appropriate action */
  if (strcmp(operation, "open") == 0)
  {
    shm_status_ptr->open_button = 1;
  }
  else if (strcmp(operation, "close") == 0)
  {
    shm_status_ptr->close_button = 1;
  }
  else if (strcmp(operation, "stop") == 0)
  {
    shm_status_ptr->emergency_stop = 1;
  }
  else if (strcmp(operation, "service_on") == 0)
  {
    shm_status_ptr->individual_service_mode = 1;
    shm_status_ptr->emergency_mode = 0;
  }
  else if (strcmp(operation, "service_off") == 0)
  {
    shm_status_ptr->individual_service_mode = 0;
  }
  else if (strcmp(operation, "up") == 0)
  {
    int destination_int = floor_char_to_int(shm_status_ptr->destination_floor);
    destination_int++;
    floor_int_to_char(destination_int, shm_status_ptr->destination_floor);
  }
  else if (strcmp(operation, "down") == 0)
  {
    int destination_int = floor_char_to_int(shm_status_ptr->destination_floor);
    destination_int--;
    floor_int_to_char(destination_int, shm_status_ptr->destination_floor);
  }

  /* finally, signal the cond and exit */
  pthread_cond_broadcast(&shm_status_ptr->cond);
  pthread_mutex_unlock(&shm_status_ptr->mutex);

  return 0;
}