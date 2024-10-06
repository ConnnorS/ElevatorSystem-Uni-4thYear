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
}