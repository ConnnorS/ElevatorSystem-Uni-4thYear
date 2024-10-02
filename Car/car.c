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
#include "car_helpers.h"
#include "../common_networks.h"

/* this runs in it's own thread, receiving any messages from
the controller and acting accordingly */
void *control_system_receive_handler(void *arg)
{
  connect_data_t *data;
  data = arg;

  while (1)
  {
    char *message = receive_message(data->socketFd);
    printf("Received message from controller: %s\n", message);
    if (message == NULL)
    {
      break;
    }
    else if (strncmp(message, "FLOOR", 5) == 0)
    {
      char floor_num[4];
      sscanf(message, "%*s %s", floor_num);
      change_floor(data, floor_num);
    }
  }

  printf("Receive handler thread ending\n");
  return NULL;
}

/* calls the control_system_receive_handler function pointer in another thread */
/* handles the connection to the control system */
void *control_system_send_handler(void *arg)
{
  /* connect to the control system */
  connect_data_t *data;
  data = arg;
  printf("Attempting to connect to control system...\n");
  while (data->socketFd == -1)
  {
    data->socketFd = connect_to_control_system();
    sleep(data->info->delay_ms / 1000); // if failed, retry after specified delay
  }
  printf("Connection successful\n");

  /* upon successful connection, spin up another thread to handle receiving messages */
  pthread_t controller_receive_thread;
  pthread_create(&controller_receive_thread, NULL, control_system_receive_handler, data);

  /* send the initial car identication data upon first connect*/
  char car_data[64];
  snprintf(car_data, sizeof(car_data), "CAR %s %s %s", data->info->name, data->info->lowest_floor, data->info->highest_floor);
  while (1)
  {
    if (send_message(data->socketFd, (char *)car_data) != -1)
      break;
    sleep(data->info->delay_ms / 1000);
  }

  /* constantly loop sending the status of the car every delay */
  while (1)
  {
    char status_data[64];
    pthread_mutex_lock(&data->status->mutex);
    snprintf(status_data, sizeof(status_data), "STATUS %s %s %s", data->status->status, data->status->current_floor, data->status->destination_floor);
    pthread_mutex_unlock(&data->status->mutex);
    while (1) // constantly loop trying to send the data
    {
      if (send_message(data->socketFd, (char *)status_data) != -1)
        break;
      sleep(data->info->delay_ms / 1000);
    }

    sleep(data->info->delay_ms / 1000);
  }

  return NULL;
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

  /* car shared memory creation for internal system */
  const int shm_status_size = sizeof(car_shared_mem);
  char *shm_status_name = car->name;
  int shm_status_fd;
  car_shared_mem *shm_status_ptr;

  shm_status_fd = do_shm_open(shm_status_name);
  do_ftruncate(shm_status_fd, shm_status_size);
  shm_status_ptr = mmap(0, shm_status_size, PROT_WRITE | PROT_READ, MAP_SHARED, shm_status_fd, 0);

  add_default_values(shm_status_ptr, lowest_floor_char);

  /* connect to control system over TCP */
  connect_data_t connect;
  connect.socketFd = -1;
  connect.info = car;
  connect.status = shm_status_ptr;
  // run all this on a seprate thread
  pthread_t controller_send_thread;
  pthread_create(&controller_send_thread, NULL, control_system_send_handler, &connect);

  // for testing
  while (1)
    ;

  /* process cleanup */
  shm_unlink(shm_status_name);
  free(car);
  return 0;
}