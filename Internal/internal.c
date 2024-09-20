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
// helper code
#include "./Car/car_helpers.h"
#include "internal_helpers.h"

/* expects {car name} {operation}*/
int main(int argc, char **argv)
{
  if (argc != 3)
  {
    printf("Useage %s {car name} {operation}\n", argv[0]);
    exit(1);
  }

  const char car_name[CAR_NAME_LENGTH] = "/car";
  const char car_operation[8];
  strcat((char *)car_name, argv[1]);
  strcpy((char *)car_operation, argv[2]);

  verify_operation((char *)car_operation);

  /* shared memory opening */
  const int shm_status_size = sizeof(car_shared_mem);
  int shm_status_fd;
  car_shared_mem *shm_status_ptr;

  shm_status_fd = shm_open((char *)car_name, O_RDWR, 0666);
  shm_status_ptr = mmap(0, shm_status_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_status_fd, 0);

  return 0;
}