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
#include "../type_conversions.h"
#include "car_status_operations.h"

/* global variables */
char *shm_status_name;
volatile sig_atomic_t system_running = 1;
volatile sig_atomic_t in_service_mode = 0;

car_shared_mem *shm_status_ptr;

pthread_t server_receive_handler;
pthread_t server_send_handler;

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
  pthread_mutex_lock(&shm_status_ptr->mutex);
  pthread_cond_broadcast(&shm_status_ptr->cond);
  pthread_mutex_unlock(&shm_status_ptr->mutex);
}

void *control_system_receive_handler(void *args)
{
  car_thread_data *car = (car_thread_data *)args;
  int fd = car->fd; // local fd variable to avoid constant mutex lock/unlock

  printf("Control system receive thread started\n");

  while (system_running && !in_service_mode)
  {
    char *message = receive_message(fd);
    if (message == NULL)
    {
      printf("Controller disconnected\n");
    }
    else if (strncmp(message, "FLOOR", 5) == 0)
    {
      char floor_num[4];
      sscanf(message, "%*s %s", floor_num);
      pthread_mutex_lock(&shm_status_ptr->mutex);
      strcpy(shm_status_ptr->destination_floor, floor_num);
      pthread_cond_broadcast(&shm_status_ptr->cond);
      pthread_mutex_unlock(&shm_status_ptr->mutex);
    }
  }
  printf("Receive thread ending - received end message\n");
  return NULL;
}

void *control_system_send_handler(void *args)
{
  printf("Control system send thread started\n");

  car_thread_data *car = (car_thread_data *)args;

  int delay_ms = car->delay_ms; // local copies of the object pointer to avoid mutexes

  /* connect to the control system */
  int socketFd = -1;
  while (socketFd == -1 && system_running)
  {
    socketFd = connect_to_control_system();
    sleep(delay_ms / 1000);
  }
  car->fd = socketFd;

  /* now create the receive thread once connected to the control system */
  pthread_create(&server_receive_handler, NULL, control_system_receive_handler, car);

  /* send the initial identification message */
  char car_data[64];
  snprintf(car_data, sizeof(car_data), "CAR %s %s %s", shm_status_name, car->lowest_floor, car->highest_floor);
  printf("Sending identification message...\n");
  while (system_running)
  {
    if (send_message(socketFd, (char *)car_data) != -1)
    {
      break;
    }
    sleep(delay_ms / 1000);
  }

  while (system_running && !in_service_mode)
  {
    /* prepare the message */
    pthread_mutex_lock(&shm_status_ptr->mutex);
    char status_data[64];
    snprintf(status_data, sizeof(status_data), "STATUS %s %s %s", shm_status_ptr->status, shm_status_ptr->current_floor, shm_status_ptr->destination_floor);
    pthread_mutex_unlock(&shm_status_ptr->mutex);
    /* send the message */
    send_message(socketFd, (char *)status_data);

    sleep(car->delay_ms / 1000);
  }

  if (in_service_mode)
  {
    send_message(socketFd, "INDIVIDUAL SERVICE");
  }
  close(socketFd);
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
  shm_status_ptr = mmap(0, sizeof(car_shared_mem), PROT_WRITE | PROT_READ, MAP_SHARED, shm_status_fd, 0);

  /* add in the default values to the shared memory object */
  pthread_mutex_lock(&shm_status_ptr->mutex);
  add_default_values(shm_status_ptr, argv[2]);
  pthread_mutex_unlock(&shm_status_ptr->mutex);

  /* create the handler threads */
  car_thread_data thread_data;
  thread_data.fd = -1;
  thread_data.delay_ms = delay_ms;
  strcpy(thread_data.lowest_floor, lowest_floor);
  strcpy(thread_data.highest_floor, highest_floor);

  pthread_create(&server_send_handler, NULL, control_system_send_handler, (void *)&thread_data);

  /* wait for commands from the internal controls or the safety system and act accordingly */
  while (system_running)
  {
    pthread_mutex_lock(&shm_status_ptr->mutex);

    /* wait while... */
    while (system_running &&                                                                // the system is running
           strcmp(shm_status_ptr->current_floor, shm_status_ptr->destination_floor) == 0 && // the car is at destination floor
           shm_status_ptr->individual_service_mode == 0)                                    // it is not in service mode
    {
      pthread_cond_wait(&shm_status_ptr->cond, &shm_status_ptr->mutex);
    }

    /* if placed into service mode */
    if (shm_status_ptr->individual_service_mode == 1)
    {
      printf("Car is in service mode\n");
      in_service_mode = 1;                                // tell threads that the car is in service mode
      if (strcmp(shm_status_ptr->status, "Between") == 0) // if status is 'Between' close the doors
      {
        closing_doors(shm_status_ptr);
        sleep(thread_data.delay_ms / 1000);
        close_doors(shm_status_ptr);
      }
      strcpy(shm_status_ptr->destination_floor, shm_status_ptr->current_floor); // set the destination floor to the current floor

      /* wait for technician's command while... */
      while (system_running &&                                                                // system is running
             strcmp(shm_status_ptr->current_floor, shm_status_ptr->destination_floor) == 0 && // the car is at destination floor
             shm_status_ptr->open_button == 0 &&                                              // the open button hasn't been pressed
             shm_status_ptr->close_button == 0 &&                                             // the close button hasn't been pressed
             shm_status_ptr->individual_service_mode == 1                                     // the car is in service mode
      )
      {
        pthread_cond_wait(&shm_status_ptr->cond, &shm_status_ptr->mutex);
      }

      /* act accordingly if any of these conditions change */
      if (strcmp(shm_status_ptr->current_floor, shm_status_ptr->destination_floor) != 0) // if the car needs to move up or down
      {
        sleep(delay_ms / 1000);
        handle_dest_floor_change(shm_status_ptr, &delay_ms);
      }
      else if (shm_status_ptr->open_button == 1) // if the open button has been pressed
      {
        opening_doors(shm_status_ptr);
        sleep(delay_ms / 1000);
        open_doors(shm_status_ptr);
        shm_status_ptr->open_button = 0;
      }
      else if (shm_status_ptr->close_button == 1) // if the close button has been pressed
      {
        closing_doors(shm_status_ptr);
        sleep(delay_ms / 1000);
        close_doors(shm_status_ptr);
        shm_status_ptr->close_button = 0;
      }
      else if (shm_status_ptr->individual_service_mode == 0) // taken out of service mode
      {
        printf("Car leaving service mode\n");
        shm_status_ptr->individual_service_mode = 0;
        in_service_mode = 0;
        pthread_create(&server_send_handler, NULL, control_system_send_handler, (void *)&thread_data); // spin up a new thread again
      }
    }

    /* if the current floor is not the destination floor - it must move up or down one */
    else if (strcmp(shm_status_ptr->current_floor, shm_status_ptr->destination_floor) != 0)
    {
      handle_dest_floor_change(shm_status_ptr, &delay_ms);
    }

    pthread_mutex_unlock(&shm_status_ptr->mutex);

    sleep(delay_ms / 1000);
  }

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