#include <string.h>

int verify_operation(char *operation)
{
  const char operations[7][12] = {"open", "close", "stop", "service_on", "service_off", "up", "down"};
  for (int i = 0; i < 5; i++)
  {
    if (strcmp(operation, operations[i]) == 0)
    {
      return 1;
    }
  }
  return 0;
}