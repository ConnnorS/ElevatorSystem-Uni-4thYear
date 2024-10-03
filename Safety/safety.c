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
// networks
#include <arpa/inet.h>
// my functions
#include "../Car/car_helpers.h"

/* expects {car name}*/
int main(int argc, char **argv)
{
  /* validate user input */
  if (argc != 2)
  {
    printf("Useage {car name}\n");
    exit(1);
  }

  char shm_status_name[CAR_NAME_LENGTH];
  snprintf(shm_status_name, sizeof(shm_status_name), "/car%s", argv[1]);

  printf("Safety system initialised for %s\n", shm_status_name);

  /* open the shared memory object */
  int shm_status_fd;
  shm_status_fd = do_shm_open(shm_status_name);
  car_shared_mem *shm_status_ptr;
  shm_status_ptr = mmap(0, sizeof(car_shared_mem), PROT_WRITE | PROT_READ, MAP_SHARED, shm_status_fd, 0);

  return 0;
}