// basic C stuff
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
char *name;
volatile sig_atomic_t system_running = 1;
volatile sig_atomic_t in_service_mode = 0;
volatile sig_atomic_t in_emergency_mode = 0;
volatile sig_atomic_t controller_connected = 1;

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

  while (system_running && !in_service_mode && !in_emergency_mode && controller_connected)
  {
    char *message = receive_message(fd);
    if (message == NULL)
    {
      printf("Controller disconnected\n");
      controller_connected = 0;
      pthread_mutex_lock(&shm_status_ptr->mutex);
      pthread_cond_broadcast(&shm_status_ptr->cond); // tell the main car thread that the controller has disconnected and to restart the connection threads
      pthread_mutex_unlock(&shm_status_ptr->mutex);
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
  printf("Receive thread ending\n");
  return NULL;
}

void *control_system_send_handler(void *args)
{
  printf("Control system send thread started\n");

  car_thread_data *car = (car_thread_data *)args;

  int delay_ms = car->delay_ms; // local copies of the object pointer to avoid mutexes

  /* connect to the control system */
  int socketFd = connect_to_control_system();
  if (socketFd == -1)
  {
    printf("Unable to connect to control system.\n");
  }
  while (socketFd == -1 && system_running)
  {
    socketFd = connect_to_control_system();
    usleep(delay_ms * 1000);
  }
  car->fd = socketFd;

  /* now create the receive thread once connected to the control system */
  pthread_create(&server_receive_handler, NULL, control_system_receive_handler, car);

  /* send the initial identification message */
  char car_data[64];
  snprintf(car_data, sizeof(car_data), "CAR %s %s %s", name, car->lowest_floor, car->highest_floor);
  printf("Sending identification message...\n");
  while (system_running)
  {
    if (send_message(socketFd, (char *)car_data) != -1)
    {
      break;
    }
    usleep(delay_ms * 1000);
  }

  while (system_running && !in_service_mode && !in_emergency_mode && controller_connected)
  {
    /* prepare the message */
    char status_data[64];
    pthread_mutex_lock(&shm_status_ptr->mutex);
    snprintf(status_data, sizeof(status_data), "STATUS %s %s %s", shm_status_ptr->status, shm_status_ptr->current_floor, shm_status_ptr->destination_floor);
    pthread_mutex_unlock(&shm_status_ptr->mutex);
    /* send the message */
    send_message(socketFd, (char *)status_data);

    /* wait for status update with a timeout */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += delay_ms / 1000;
    ts.tv_nsec += (delay_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000)
    {
      ts.tv_sec += 1;
      ts.tv_nsec -= 1000000000;
    }
    pthread_mutex_lock(&shm_status_ptr->mutex);
    pthread_cond_timedwait(&shm_status_ptr->cond, &shm_status_ptr->mutex, &ts);
    pthread_mutex_unlock(&shm_status_ptr->mutex);
  }

  if (in_service_mode)
  {
    send_message(socketFd, "INDIVIDUAL SERVICE");
  }
  else if (in_emergency_mode)
  {
    send_message(socketFd, "EMERGENCY");
  }

  /* wait for the receive handler to close then close everything off */
  pthread_join(server_receive_handler, NULL);
  close(socketFd);
  printf("Send thread ending\n");
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

  int delay_ms = atoi(argv[4]);

  /* validate the user input */
  int lowest_floor_int = floor_char_to_int(argv[2]);
  int highest_floor_int = floor_char_to_int(argv[3]);
  validate_floor_range(lowest_floor_int);
  validate_floor_range(highest_floor_int);
  compare_highest_lowest(lowest_floor_int, highest_floor_int);

  /* save the name of the car */
  name = malloc(64);
  strcpy(name, argv[1]);

  /* create the shared memory object for the car */
  shm_status_name = malloc(64);
  snprintf(shm_status_name, 64, "/car%s", argv[1]);
  shm_unlink(shm_status_name); // unlink previous memory just in case
  int shm_status_fd = do_shm_open(shm_status_name);
  do_ftruncate(shm_status_fd, sizeof(car_shared_mem));
  shm_status_ptr = mmap(0, sizeof(car_shared_mem), PROT_WRITE, MAP_SHARED, shm_status_fd, 0);

  /* add in the default values to the shared memory object */
  init_shared_mem(shm_status_ptr, argv[2]);

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
           strcmp(shm_status_ptr->current_floor, shm_status_ptr->destination_floor) == 0 && // the car is at its destination floor
           !shm_status_ptr->individual_service_mode &&                                      // it is not in service mode
           !shm_status_ptr->emergency_mode &&                                               // it is not in emergency mode
           controller_connected &&                                                          // the controller is still connected
           shm_status_ptr->open_button == 0 &&                                              // open button hasn't been pressed
           shm_status_ptr->close_button == 0)                                               // close button hasn't been pressed
    {
      pthread_cond_wait(&shm_status_ptr->cond, &shm_status_ptr->mutex);
    }

    /* if placed into service mode */
    if (shm_status_ptr->individual_service_mode == 1)
    {
      printf("Car is in service mode\n");
      if (strcmp(shm_status_ptr->status, "Between") == 0 && in_service_mode == 0) // if status is 'Between' close the doors on initial move into service mode
      {
        closing_doors(shm_status_ptr);
        usleep(thread_data.delay_ms * 1000);
        close_doors(shm_status_ptr, &delay_ms);
      }
      in_service_mode = 1; // tell threads that the car is in service mode

      strcpy(shm_status_ptr->destination_floor, shm_status_ptr->current_floor); // set the destination floor to the current floor

      /* wait for technician's command in service mode while... */
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
        usleep(delay_ms * 1000);
        handle_dest_floor_change(shm_status_ptr, &delay_ms, &lowest_floor_int, &highest_floor_int);
      }
      else if (shm_status_ptr->open_button == 1) // if the open button has been pressed
      {
        opening_doors(shm_status_ptr);
        usleep(delay_ms * 1000);
        open_doors(shm_status_ptr);
        shm_status_ptr->open_button = 0;
      }
      else if (shm_status_ptr->close_button == 1) // if the close button has been pressed
      {
        closing_doors(shm_status_ptr);
        usleep(delay_ms * 1000);
        close_doors(shm_status_ptr, &delay_ms);
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

    /* if placed into emergency mode */
    else if (shm_status_ptr->emergency_mode == 1)
    {
      printf("Car is in emergency mode\n");
      in_emergency_mode = 1;

      strcpy(shm_status_ptr->destination_floor, shm_status_ptr->current_floor); // set the destination floor to the current floor

      /* wait for a technician to take the car out of emergency mode and into service mode */
      while (shm_status_ptr->open_button == 0 &&          // the open button hasn't been pressed
             shm_status_ptr->close_button == 0 &&         // the close button hasn't been pressed
             shm_status_ptr->individual_service_mode == 0 // the technician hasn't put the car in service mode yet
      )
      {
        pthread_cond_wait(&shm_status_ptr->cond, &shm_status_ptr->mutex);
      }

      if (shm_status_ptr->emergency_mode == 0)
      {
        printf("Car leaving emergency mode\n");
        in_emergency_mode = 0;
      }
      else if (shm_status_ptr->open_button == 1) // if the open button has been pressed
      {
        opening_doors(shm_status_ptr);
        usleep(delay_ms * 1000);
        open_doors(shm_status_ptr);
        shm_status_ptr->open_button = 0;
      }
      else if (shm_status_ptr->close_button == 1) // if the close button has been pressed
      {
        closing_doors(shm_status_ptr);
        usleep(delay_ms * 1000);
        close_doors(shm_status_ptr, &delay_ms);
        shm_status_ptr->close_button = 0;
      }
    }

    /* if the current floor is not the destination floor */
    else if (strcmp(shm_status_ptr->current_floor, shm_status_ptr->destination_floor) != 0)
    {
      handle_dest_floor_change(shm_status_ptr, &delay_ms, &lowest_floor_int, &highest_floor_int);
    }

    /* if the controller disconnects - restart the connection threads to connect again */
    else if (!controller_connected)
    {
      pthread_join(server_send_handler, NULL);
      controller_connected = 1;
      pthread_create(&server_send_handler, NULL, control_system_send_handler, (void *)&thread_data); // spin up a new thread again
    }

    /* if the open button is pressed */
    else if (shm_status_ptr->open_button == 1)
    {
      shm_status_ptr->open_button = 0;

      if (strcmp(shm_status_ptr->status, "Open") == 0)
      {
        pthread_mutex_unlock(&shm_status_ptr->mutex);
        usleep(delay_ms * 1000);
        pthread_mutex_lock(&shm_status_ptr->mutex);

        closing_doors(shm_status_ptr);

        pthread_mutex_unlock(&shm_status_ptr->mutex);
        usleep(delay_ms * 1000);
        pthread_mutex_lock(&shm_status_ptr->mutex);

        close_doors(shm_status_ptr, &delay_ms);
      }
      else if (strcmp(shm_status_ptr->status, "Closing") == 0 || strcmp(shm_status_ptr->status, "Closed") == 0)
      {
        opening_doors(shm_status_ptr);

        pthread_mutex_unlock(&shm_status_ptr->mutex);
        usleep(delay_ms * 1000);
        pthread_mutex_lock(&shm_status_ptr->mutex);

        open_doors(shm_status_ptr);

        pthread_mutex_unlock(&shm_status_ptr->mutex);
        usleep(delay_ms * 1000);
        pthread_mutex_lock(&shm_status_ptr->mutex);

        closing_doors(shm_status_ptr);

        pthread_mutex_unlock(&shm_status_ptr->mutex);
        usleep(delay_ms * 1000);
        pthread_mutex_lock(&shm_status_ptr->mutex);

        close_doors(shm_status_ptr, &delay_ms);
      }
    }

    /* if the close button is pressed and the doors are open */
    else if (shm_status_ptr->close_button == 1 && strcmp(shm_status_ptr->status, "Open") == 0)
    {
      shm_status_ptr->close_button = 0;

      closing_doors(shm_status_ptr);

      pthread_mutex_unlock(&shm_status_ptr->mutex);
      usleep(delay_ms * 1000);
      pthread_mutex_lock(&shm_status_ptr->mutex);

      close_doors(shm_status_ptr, &delay_ms);
    }

    pthread_mutex_unlock(&shm_status_ptr->mutex);
  }

  /* wait for the threads to end - this will happen when system_running is set
  to 0 with CTRL + C */
  pthread_join(server_receive_handler, NULL);
  pthread_join(server_send_handler, NULL);

  pthread_mutex_destroy(&shm_status_ptr->mutex);
  pthread_cond_destroy(&shm_status_ptr->cond);
  printf("Destroyed mutexes and conds\n");

  /* do the cleanup when the threads end */
  if (shm_unlink(shm_status_name) == -1)
  {
    perror("shm_unlink()");
  }

  free(shm_status_name);
  free(name);

  printf("Unlinked and freed memory\n");

  return 0;
}