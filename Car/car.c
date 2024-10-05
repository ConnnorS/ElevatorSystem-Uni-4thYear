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
// comms
#include "../common_comms.h"

/* global variables */
char *shm_status_name;
volatile sig_atomic_t system_running = 1;

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

void thread_cleanup(int signal)
{
  system_running = 0;
}

void *control_system_receive_handler(void *args)
{
  car_thread_data *client = (car_thread_data *)args;
  pthread_mutex_lock(&client->mutex);
  int fd = client->fd; // local fd variable to avoid constant mutex lock/unlock
  pthread_mutex_unlock(&client->mutex);

  printf("Control system receive thread started\n");

  while (system_running)
  {
    char *message = receive_message(fd);
    if (message == NULL)
    {
      printf("Controller disconnected\n");
      system_running = 0;
    }
    else if (strcmp(message, "") == 0)
    {
      continue;
    }
    else
    {
      printf("%s\n", message);
    }
  }
  printf("Receive thread ending - received end message\n");
  return NULL;
}

void *control_system_send_handler(void *args)
{
  car_thread_data *client = (car_thread_data *)args;
  pthread_mutex_lock(&client->mutex);
  int fd = client->fd; // local fd variable to avoid constant mutex lock/unlock
  pthread_mutex_unlock(&client->mutex);

  printf("Control system send thread started\n");

  while (system_running)
  {
    /* prepare the message */
    pthread_mutex_lock(&client->ptr->mutex);
    char status_data[64];
    snprintf(status_data, sizeof(status_data), "STATUS %s %s %s", client->ptr->status, client->ptr->current_floor, client->ptr->destination_floor);
    pthread_mutex_unlock(&client->ptr->mutex);
    /* send the message */
    send_message(fd, (char *)status_data);

    sleep(client->delay_ms / 1000);
  }
  printf("Send thread ending - received end message\n");
  return NULL;
}

int main(int argc, char **argv)
{
  signal(SIGINT, thread_cleanup);

  if (argc != 5)
  {
    printf("Useage %s {name} {lowest floor} {highest floor} {delay}\n", argv[0]);
    exit(1);
  }

  char lowest_floor[4];
  char highest_floor[4];
  strcpy(lowest_floor, argv[2]);
  strcpy(highest_floor, argv[3]);

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
  while (socketFd == -1 && system_running)
  {
    socketFd = connect_to_control_system();
    sleep(delay_ms / 1000);
  }
  /* send the initial identification message */
  char car_data[64];
  pthread_mutex_lock(&shm_status_ptr->mutex);
  snprintf(car_data, sizeof(car_data), "CAR %s %s %s", shm_status_name, lowest_floor, highest_floor);
  pthread_mutex_unlock(&shm_status_ptr->mutex);
  printf("Sending identification message...\n");
  while (system_running)
  {
    if (send_message(socketFd, (char *)car_data) != -1)
    {
      break;
    }
    sleep(delay_ms / 1000);
  }

  /* create the handler threads */
  car_thread_data thread_data;
  thread_data.fd = socketFd;
  thread_data.ptr = shm_status_ptr;
  thread_data.delay_ms = delay_ms;
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&thread_data.mutex, &attr);

  pthread_t server_send_handler;
  pthread_create(&server_send_handler, NULL, control_system_send_handler, (void *)&thread_data);

  pthread_t server_receive_handler;
  pthread_create(&server_receive_handler, NULL, control_system_receive_handler, (void *)&thread_data);

  /* wait for the threads to end - this will happen when system_running is set
  to 0 with CTRL + C */
  pthread_join(server_receive_handler, NULL);
  pthread_join(server_send_handler, NULL);

  /* do the cleanup when the threads end */
  if (shm_unlink(shm_status_name) == -1)
  {
    perror("shm_unlink()");
  }

  free(shm_status_name);

  printf("Unlinked and freed memory\n");

  return 0;
}