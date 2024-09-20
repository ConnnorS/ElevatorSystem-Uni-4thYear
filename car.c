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

void print_car_info(const car_info *car)
{
  printf("CAR INFO:\n");
  printf("Car Name: %s\n", car->name);
  printf("Lowest Floor: %s\n", car->lowest_floor);
  printf("Highest Floor: %s\n", car->highest_floor);
  printf("Delay (ms): %d\n", car->delay_ms);
  printf("\n");
}

/* expected CL-arguments
{name} {lowest floor} {highest floor} {delay} */
int main(int argc, char **argv)
{
  if (argc != 5)
  {
    printf("Useage %s {name} {lowest floor} {highest floor} {delay}\n", argv[0]);
    exit(1);
  }

  car_info *car = malloc(sizeof(car_info));

  // save the user inputs into a char before we convert to negative number
  const char lowest_floor_char[4];
  strcpy((char *)lowest_floor_char, argv[2]);
  const char highest_floor_char[4];
  strcpy((char *)highest_floor_char, argv[3]);
  // convert the basement floors to a negative number for easier comparison
  if (argv[2][0] == 'B')
    argv[2][0] = '-';
  if (argv[3][0] == 'B')
    argv[3][0] = '-';

  // validate the user input
  const int lowest_floor = atoi(argv[2]);
  const int highest_floor = atoi(argv[3]);
  validate_floor_range(lowest_floor);
  validate_floor_range(highest_floor);
  compare_highest_lowest(lowest_floor, highest_floor);

  // add the information to the car_info struct
  strcpy(car->name, "/car");
  strcat(car->name, argv[1]);
  strcpy(car->lowest_floor, lowest_floor_char);
  strcpy(car->highest_floor, highest_floor_char);
  car->delay_ms = atoi(argv[4]);

  // for testing
  print_car_info(car);

  /* car shared memory creation for internal system */
  // setup the names and info
  const int shm_status_size = sizeof(car_shared_mem);
  char *shm_status_name = car->name;
  int shm_status_fd;
  car_shared_mem *shm_status_ptr;

  // create the shared memory object
  shm_status_fd = shm_open(shm_status_name, O_CREAT | O_RDWR, 0666);
  // set the size of the shared memory object
  ftruncate(shm_status_fd, shm_status_size);
  // map the shared object
  shm_status_ptr = mmap(0, shm_status_size, PROT_WRITE, MAP_SHARED, shm_status_fd, 0);

  // add in default values into the shared memory object
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

  // for testing
  print_car_shared_mem(shm_status_ptr, shm_status_name);

  // for testing
  sleep(10);

  /* process cleanup */
  shm_unlink(shm_status_name);
  free(car);
  return 0;
}