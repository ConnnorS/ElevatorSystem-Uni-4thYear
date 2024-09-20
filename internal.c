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
// struct definitions
#include "car_helpers.h"

void print_car_shared_mem(const car_shared_mem *shared_mem, const char *name)
{
  if (shared_mem == NULL)
  {
    printf("Shared memory pointer is NULL\n");
    return;
  }

  // Since `pthread_mutex_t` and `pthread_cond_t` are not printable, we'll skip them
  printf("SHARED MEM INFO:\n");
  printf("Shared mem name: %s\n", name);
  printf("Current Floor: %s\n", shared_mem->current_floor);
  printf("Destination Floor: %s\n", shared_mem->destination_floor);
  printf("Status: %s\n", shared_mem->status);
  printf("Open Button: %d\n", shared_mem->open_button);
  printf("Close Button: %d\n", shared_mem->close_button);
  printf("Door Obstruction: %d\n", shared_mem->door_obstruction);
  printf("Overload: %d\n", shared_mem->overload);
  printf("Emergency Stop: %d\n", shared_mem->emergency_stop);
  printf("Individual Service Mode: %d\n", shared_mem->individual_service_mode);
  printf("Emergency Mode: %d\n", shared_mem->emergency_mode);
  printf("\n");
}

void verify_operation(const char operation[8])
{
  const char operations[5][8] = {"Opening", "Open", "Closed", "Closing", "Between"};
  for (int i = 0; i < 5; i++)
  {
    if (strcmp(operation, operations[i]) == 0)
    {
      return;
    }
  }

  printf("Invalid operation specified\nPlease enter \"Opening\", \"Open\", \"Closed\", \"Closing\", or \"Between\"\n");
  exit(1);
}

void print_operation_info(char car_name[CAR_NAME_LENGTH], char car_operation[8])
{
  printf("Car: %s\n", car_name);
  printf("Operation: %s\n", car_operation);
}

/* expectes {car name} {operation}*/
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

  print_operation_info((char *)car_name, (char *)car_operation);

  /* shared memory opening */
  const int shm_status_size = sizeof(car_shared_mem);
  int shm_status_fd;
  car_shared_mem *shm_status_ptr;

  shm_status_fd = shm_open((char *)car_name, O_RDWR, 0666);
  shm_status_ptr = mmap(0, shm_status_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_status_fd, 0);

  print_car_shared_mem(shm_status_ptr, car_name);

  return 0;
}