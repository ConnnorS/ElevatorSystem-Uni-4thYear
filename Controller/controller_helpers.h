#include "./Car/car_helpers.h"

typedef struct car_status
{
  char name[CAR_NAME_LENGTH];
  char status[8];
  char current_floor[4];
  char destination_floor[4];
} car_status_t;

int create_server();
char *receive_message(int fd);
void handle_received_status_message(car_status_t *car_status_list);