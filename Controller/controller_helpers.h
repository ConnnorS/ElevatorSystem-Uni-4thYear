#define CAR_NAME_LENGTH 32

typedef struct car_status
{
  char name[CAR_NAME_LENGTH];
  char status[8];
  char current_floor[4];
  char destination_floor[4];
} car_status_info;

typedef struct
{
  char name[CAR_NAME_LENGTH];
  char lowest_floor[4];
  char highest_floor[4];
} controller_car_info;

int create_server();
char *receive_message(int fd);
void handle_received_car_message(controller_car_info **car_list, char *message, int *count);
void handle_received_status_message(car_status_info **car_status_list, char *message, int *count);