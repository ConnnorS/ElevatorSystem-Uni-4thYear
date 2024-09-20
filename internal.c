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
// struct definitions
#include "car_helpers.h"

void verify_operation(const char operation[8])
{
  const char operations[5][8] = {"Opening", "Open", "Closed", "Closing", "Between"};
  for (int i = 0; i < 5; i++)
  {
    if (strcmp(operation, operations[i]) == 0)
    {
      return;
    }
  }

  printf("Invalid operation specified\nPlease enter \"Opening\", \"Open\", \"Closed\", \"Closing\", or \"Between\"\n");
  exit(1);
}

void print_operation_info(char car_name[32], char car_operation[8])
{
  printf("Car: %s\n", car_name);
  printf("Operation: %s\n", car_operation);
}

/* expectes {car name} {operation}*/
int main(int argc, char **argv)
{
  if (argc != 3)
  {
    printf("Useage %s {car name} {operation}\n", argv[0]);
    exit(1);
  }

  const char car_name[32] = "/car";
  const char car_operation[8];
  strcat((char *)car_name, argv[1]);
  strcpy((char *)car_operation, argv[2]);

  verify_operation((char *)car_operation);

  print_operation_info((char *)car_name, (char *)car_operation);

  return 0;
}