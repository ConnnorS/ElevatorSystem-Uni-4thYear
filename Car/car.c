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
// signal
#include <signal.h>
// headers
#include "car_helpers.h"

/* global variables */
char *shm_status_name;

void print_car_shared_mem(const car_shared_mem *car)
{
  printf("-- NEW UPDATE -- \n\n");
  printf("Current Floor: %s\n", car->current_floor);
  printf("Destination Floor: %s\n", car->destination_floor);
  printf("Status: %s\n", car->status);
  printf("Open Button: %u\n", car->open_button);
  printf("Close Button: %u\n", car->close_button);
  printf("Door Obstruction: %u\n", car->door_obstruction);
  printf("Overload: %u\n", car->overload);
  printf("Emergency Stop: %u\n", car->emergency_stop);
  printf("Individual Service Mode: %u\n", car->individual_service_mode);
  printf("Emergency Mode: %u\n", car->emergency_mode);
}

void exit_cleanup(int signal)
{
  printf("Received exit code %d\n", signal);
  printf("Cleaning up...\n");

  if (shm_unlink(shm_status_name) == -1)
  {
    perror("shm_unlink()");
    return;
  }

  free(shm_status_name);
  printf("Cleanup completed\n");
  exit(0);
}

int main(int argc, char **argv)
{
  signal(SIGINT, exit_cleanup);

  if (argc != 5)
  {
    printf("Useage %s {name} {lowest floor} {highest floor} {delay}\n", argv[0]);
    exit(1);
  }

  // convert the basement floors to a negative number for easier comparison
  if (argv[2][0] == 'B')
    argv[2][0] = '-';
  if (argv[3][0] == 'B')
    argv[3][0] = '-';

  int delay_ms = atoi(argv[4]);

  /* validate the user input */
  const int lowest_floor_int = atoi(argv[2]);
  const int highest_floor_int = atoi(argv[3]);
  validate_floor_range(lowest_floor_int);
  validate_floor_range(highest_floor_int);
  compare_highest_lowest(lowest_floor_int, highest_floor_int);

  /* create the shared memory object for the car */
  shm_status_name = malloc(64);
  snprintf(shm_status_name, sizeof(shm_status_name), "/car%s", argv[1]);
  int shm_status_fd = do_shm_open(shm_status_name);
  do_ftruncate(shm_status_fd, sizeof(car_shared_mem));
  car_shared_mem *shm_status_ptr = mmap(0, sizeof(car_shared_mem), PROT_WRITE | PROT_READ, MAP_SHARED, shm_status_fd, 0);

  /* add in the default values to the shared memory object */
  pthread_mutex_lock(&shm_status_ptr->mutex);
  add_default_values(shm_status_ptr, argv[2]);
  pthread_mutex_unlock(&shm_status_ptr->mutex);

  /* connect to the control system */
  int socketFd = -1;
  while (socketFd == -1)
  {
    socketFd = connect_to_control_system();
    sleep(delay_ms / 1000);
  }

  exit_cleanup(1);
  return 0;
}