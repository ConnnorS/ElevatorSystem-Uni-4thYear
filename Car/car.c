// basic C
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// signal
#include <signal.h>
// threads
#include <pthread.h>
// shared mem
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "car_helpers.h"
#include "../type_conversions.h"
#include "../common_comms.h"
#include "car_status_operations.h"

/* global variables */
char *shm_status_name;                          // name of shared mem
char *name;                                     // name of car
volatile sig_atomic_t system_running = 1;       // allows the entire car component to shut down
volatile sig_atomic_t in_service_mode = 0;      // tells the threads if the car is in service mode
volatile sig_atomic_t in_emergency_mode = 0;    // tells the threads if the car is in emergency mode
volatile sig_atomic_t controller_connected = 1; // tells the threads if the controller is connected
car_shared_mem *shm_status_ptr;                 // pointer to shared mem
pthread_t server_receive_handler;
pthread_t server_send_handler;

/* function declarations */
void system_shutdown();
void *control_system_send_handler(void *args);
void *control_system_receive_handler(void *args);

/* {car name} {lowest floor} {highest floor} {delay} */
int main(int argc, char **argv)
{
  signal(SIGINT, system_shutdown);

  if (argc != 5)
  {
    printf("Useage %s {name} {lowest floor} {highest floor} {delay}\n", argv[0]);
    return 1;
  }

  /* grab user input */
  char lowest_floor_char[4];
  char highest_floor_char[4];
  strncpy(lowest_floor_char, argv[2], 4);
  strncpy(highest_floor_char, argv[3], 4);

  char *end_ptr;
  __useconds_t delay_ms = (__useconds_t)strtoul(argv[4], &end_ptr, 10);
  if (*end_ptr != '\0')
  {
    printf("Invalid delay_ms specified\n");
    return 1;
  }

  /* validate user input */
  int lowest_floor_int = floor_char_to_int(lowest_floor_char);
  int highest_floor_int = floor_char_to_int(highest_floor_char);
  if (validate_floor_range(lowest_floor_int) == -1 || validate_floor_range(highest_floor_int) == -1)
  {
    printf("Invalid floor(s) specified\n");
    return 1;
  }
  if (lowest_floor_int > highest_floor_int)
  {
    printf("Invalid floor(s) specified\n");
    return 1;
  }

  /* save the name */
  name = malloc(64);
  strcpy(name, argv[1]);

  /* create the shared memory object */
  shm_status_name = malloc(64);
  snprintf(shm_status_name, 64, "/car%s", argv[1]);
  shm_unlink(shm_status_name); // unlink previous memory just in case
  int shm_status_fd = do_shm_open(shm_status_name);
  do_ftruncate(shm_status_fd, sizeof(car_shared_mem));
  shm_status_ptr = mmap(0, sizeof(car_shared_mem), PROT_WRITE, MAP_SHARED, shm_status_fd, 0);
  init_shared_mem(shm_status_ptr, lowest_floor_char);

  /* create the data for the threads */
  car_thread_data *thread_data = malloc(sizeof(car_thread_data));
  thread_data->fd = -1;
  thread_data->delay_ms = delay_ms;
  strncpy(thread_data->lowest_floor, lowest_floor_char, 4);
  strncpy(thread_data->highest_floor, highest_floor_char, 4);

  pthread_create(&server_send_handler, NULL, control_system_send_handler, thread_data);

  /* wait for commands from internal controls or safety system */
  while (system_running)
  {
    pthread_mutex_lock(&shm_status_ptr->mutex);

    /* service mode wait */
    if (in_service_mode)
    {
      while (system_running &&                                                                // system is running
             strcmp(shm_status_ptr->current_floor, shm_status_ptr->destination_floor) == 0 && // the car is at destination floor
             shm_status_ptr->open_button == 0 &&                                              // the open button hasn't been pressed
             shm_status_ptr->close_button == 0 &&                                             // the close button hasn't been pressed
             shm_status_ptr->individual_service_mode == 1                                     // the car is in service mode
      )
      {
        pthread_cond_wait(&shm_status_ptr->cond, &shm_status_ptr->mutex);
      }
    }
    /* emergency mode wait */
    else if (in_emergency_mode)
    {
      while (system_running &&                            // system is running
             shm_status_ptr->open_button == 0 &&          // the open button hasn't been pressed
             shm_status_ptr->close_button == 0 &&         // the close button hasn't been pressed
             shm_status_ptr->individual_service_mode == 0 // the car is not in service mode
      )
      {
        pthread_cond_wait(&shm_status_ptr->cond, &shm_status_ptr->mutex);
      }
    }
    /* normal conditions wait */
    else
    {
      while (system_running &&                                                                // the system is running
             !shm_status_ptr->individual_service_mode &&                                      // it is not in service mode
             strcmp(shm_status_ptr->current_floor, shm_status_ptr->destination_floor) == 0 && // the car is at its destination floor
             shm_status_ptr->open_button == 0 &&                                              // open button hasn't been pressed
             shm_status_ptr->close_button == 0 &&                                             // close button hasn't been pressed
             !shm_status_ptr->emergency_mode &&                                               // it is not in emergency mode
             controller_connected                                                             // the controller is still connected
      )
      {
        pthread_cond_wait(&shm_status_ptr->cond, &shm_status_ptr->mutex);
      }
    }

    /* if the car is placed into service mode for the first time */
    if (shm_status_ptr->individual_service_mode && !in_service_mode)
    {
      if (in_emergency_mode)
      {
        shm_status_ptr->emergency_mode = 0;
        in_emergency_mode = 0;
      }
      handle_service_mode(shm_status_ptr);
      in_service_mode = 1;
    }
    /* if internals tells the car to leave service mode */
    else if (!shm_status_ptr->individual_service_mode && in_service_mode)
    {
      in_service_mode = 0;
      pthread_create(&server_send_handler, NULL, control_system_send_handler, thread_data); // spin up a new thread again
    }
    /* if placed into emergency mode for the first time */
    else if (shm_status_ptr->emergency_mode && !in_emergency_mode)
    {
      in_emergency_mode = 1;
      shm_status_ptr->emergency_stop = 0;
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
      }
      /* if not in service mode */
      else if ((strcmp(shm_status_ptr->status, "Closed") == 0 || strcmp(shm_status_ptr->status, "Closing") == 0) && !in_service_mode)
      {
        door_open_close(shm_status_ptr, &delay_ms);
      }
      /* if in service mode */
      else if ((strcmp(shm_status_ptr->status, "Closed") == 0 || strcmp(shm_status_ptr->status, "Closing") == 0) && in_service_mode)
      {
        opening_doors(shm_status_ptr);
        pthread_mutex_unlock(&shm_status_ptr->mutex);
        usleep(delay_ms * 1000);
        pthread_mutex_lock(&shm_status_ptr->mutex);
        open_doors(shm_status_ptr);
      }
    }
    /* if the close button is pressed */
    else if (shm_status_ptr->close_button == 1)
    {
      shm_status_ptr->close_button = 0;

      if (strcmp(shm_status_ptr->status, "Open") == 0)
      {
        closing_doors(shm_status_ptr, &delay_ms);

        pthread_mutex_unlock(&shm_status_ptr->mutex);
        usleep(delay_ms * 1000);
        pthread_mutex_lock(&shm_status_ptr->mutex);

        close_doors(shm_status_ptr);
      }
    }
    /* if dest is different to current */
    else if (strcmp(shm_status_ptr->current_floor, shm_status_ptr->destination_floor) != 0)
    {
      handle_dest_floor_change(shm_status_ptr, &delay_ms, &lowest_floor_int, &highest_floor_int);
    }
    pthread_mutex_unlock(&shm_status_ptr->mutex);
  }

  /* wait for the threads to end - this will happen when system_running is set
  to 0 with CTRL + C */
  pthread_join(server_receive_handler, NULL);
  pthread_join(server_send_handler, NULL);

  pthread_mutex_destroy(&shm_status_ptr->mutex);
  pthread_cond_destroy(&shm_status_ptr->cond);

  /* do the cleanup when the threads end */
  if (shm_unlink(shm_status_name) == -1)
  {
    perror("shm_unlink()");
  }

  free(thread_data);
  free(shm_status_name);
  free(name);
  return 0;
}

/* handles sending messages to the controller */
void *control_system_send_handler(void *args)
{

  car_thread_data *car = (car_thread_data *)args;
  __useconds_t delay_ms = car->delay_ms;

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
  char car_id[64];
  snprintf(car_id, sizeof(car_id), "CAR %s %s %s", name, car->lowest_floor, car->highest_floor);
  while (system_running)
  {
    if (send_message(socketFd, (char *)car_id) != -1)
    {
      break;
    }
    usleep(delay_ms * 1000);
  }

  /* constantly send status messages every delay_ms or when data changes */
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
  return NULL;
}

/* handles all received connections from the controller */
void *control_system_receive_handler(void *args)
{
  car_thread_data *car = (car_thread_data *)args;
  int fd = car->fd;


  while (system_running && !in_service_mode && !in_emergency_mode && controller_connected)
  {
    char *message = receive_message(fd);
    /* tell the other thread to shutdown - the controller has disconnected */
    if (message == NULL)
    {
      controller_connected = 0;
      pthread_mutex_lock(&shm_status_ptr->mutex);
      pthread_cond_broadcast(&shm_status_ptr->cond);
      pthread_mutex_unlock(&shm_status_ptr->mutex);
    }
    /* update the destination floor for the main thread to then move floors */
    else if (strncmp(message, "FLOOR", 5) == 0)
    {
      char floor_num[4];
      sscanf(message, "%*s %s", floor_num);
      pthread_mutex_lock(&shm_status_ptr->mutex);
      if (strcmp(floor_num, shm_status_ptr->current_floor) == 0)
      {
        shm_status_ptr->open_button = 1;
      }
      strcpy(shm_status_ptr->destination_floor, floor_num);
      pthread_cond_broadcast(&shm_status_ptr->cond);
      pthread_mutex_unlock(&shm_status_ptr->mutex);
    }
  }
  return NULL;
}

/* allows everything to cleanly shut down */
void system_shutdown()
{
  system_running = 0;
  pthread_mutex_lock(&shm_status_ptr->mutex);
  pthread_cond_broadcast(&shm_status_ptr->cond);
  pthread_mutex_unlock(&shm_status_ptr->mutex);
}