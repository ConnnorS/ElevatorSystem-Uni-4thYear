#include <string.h>

int verify_operation(char *operation)
{
  const char operations[10][12] = {"open", "close", "stop", "service_on", "service_off", "up", "down", "emrg", "obs_on", "obs_off"};
  for (int i = 0; i < 10; i++)
  {
    if (strcmp(operation, operations[i]) == 0)
    {
      return 1;
    }
  }
  return 0;
}