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
#include "../common_comms.h"
#include "../type_conversions.h"
#include "../common_headers.h"

/* {source floor} {destination floor} */
int main(int argc, char **argv)
{
  /* check user input */
  if (argc != 3)
  {
    printf("Useage %s {source floor} {destination floor}\n", argv[0]);
    exit(1);
  }

  char source_floor[FLOOR_LEN];
  strcpy(source_floor, argv[1]);
  char destination_floor[FLOOR_LEN];
  strcpy(destination_floor, argv[2]);

  /* validate floor ranges */
  int source_floor_int = floor_char_to_int(source_floor);
  int destination_floor_int = floor_char_to_int(destination_floor);

  if (validate_floor_range(source_floor_int) == -1 || validate_floor_range(destination_floor_int) == -1)
  {
    printf("Invalid floor(s) specified.\n");
    exit(1);
  }

  if (strcmp(source_floor, destination_floor) == 0)
  {
    printf("You are already on that floor!\n");
    exit(1);
  }

  /* connect to the control system */
  int serverFd = connect_to_control_system();
  if (serverFd == -1)
  {
    printf("Unable to connect to elevator system.\n");
    exit(1);
  }

  /* prepare and send the message */
  char message[MESSAGE_LEN];
  snprintf(message, sizeof(message), "CALL %s %s", source_floor, destination_floor);
  send_message(serverFd, message);

  /* await a response */
  char *response = receive_message(serverFd);
  if (strcmp(response, "UNAVAILABLE") == 0)
  {
    printf("Sorry, no car is available to take this request.\n");
  }
  else
  {
    printf("%s is arriving.\n", response);
  }
  return 0;
}