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
// my headers
#include "car_helpers.h"
// comms
#include "../common_comms.h"

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

/* simple function to set up a TCP connection with the controller
then return an int which is the socketFd */
int connect_to_control_system()
{
  // create the address
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) == -1)
  {
    perror("inet_pton()");
    return -1;
  }
  addr.sin_family = AF_INET;
  addr.sin_port = htons(3000);

  // create the socket
  int socketFd = socket(AF_INET, SOCK_STREAM, 0);
  if (socketFd == -1)
  {
    perror("socket()");
    return -1;
  }

  // connect to the server
  if (connect(socketFd, (const struct sockaddr *)&addr, sizeof(addr)) == -1)
  {
    perror("connect()");
    return -1;
  }

  return socketFd;
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

void validate_floor_range(int floor)
{
  if (floor < -99)
  {
    printf("Lowest or highest floor cannot be lower than B99\n");
    exit(1);
  }
  else if (floor > 999)
  {
    printf("Lowest or highest floor cannot be higher than 999\n");
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

void add_default_values(car_shared_mem *shm_status_ptr, const char *lowest_floor_char)
{
  // initialise the shared mutex
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&shm_status_ptr->mutex, &attr);
  // initialise the shared condition variable
  pthread_condattr_t cond_attr;
  pthread_condattr_init(&cond_attr);
  pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(&shm_status_ptr->cond, &cond_attr);

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