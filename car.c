#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include "car_helpers.h"

typedef struct
{
  pthread_mutex_t mutex;           // Locked while accessing struct contents
  pthread_cond_t cond;             // Signalled when the contents change
  char current_floor[4];           // C string in the range B99-B1 and 1-999
  char destination_floor[4];       // Same format as above
  char status[8];                  // C string indicating the elevator's status
  uint8_t open_button;             // 1 if open doors button is pressed, else 0
  uint8_t close_button;            // 1 if close doors button is pressed, else 0
  uint8_t door_obstruction;        // 1 if obstruction detected, else 0
  uint8_t overload;                // 1 if overload detected
  uint8_t emergency_stop;          // 1 if stop button has been pressed, else 0
  uint8_t individual_service_mode; // 1 if in individual service mode, else 0
  uint8_t emergency_mode;          // 1 if in emergency mode, else 0
} car_shared_mem;

typedef struct
{
  char name[32];
  int lowest_floor;
  int highest_floor;
  int delay_ms;
} car_info;

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

  // convert the basement floors to a negative number for easier comparison
  if (argv[2][0] == 'B')
    argv[2][0] = '-';
  if (argv[3][0] == 'B')
    argv[3][0] = '-';

  const int lowest_floor = atoi(argv[2]);
  const int highest_floor = atoi(argv[3]);
  validate_floor_range(lowest_floor);
  validate_floor_range(highest_floor);
  compare_highest_lowest(lowest_floor, highest_floor);

  strcpy(car->name, argv[1]);
  car->lowest_floor = lowest_floor;
  car->highest_floor = highest_floor;
  car->delay_ms = atoi(argv[4]);

  printf("Car Name: %s\n", car->name);
  printf("Lowest Floor: %d\n", car->lowest_floor);
  printf("Highest Floor: %d\n", car->highest_floor);
  printf("Delay (ms): %d\n", car->delay_ms);

  free(car);
  return 0;
}