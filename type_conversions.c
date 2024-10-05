#include <stdio.h>
#include <stdlib.h>

int floor_char_to_int(char *floor)
{
  if (floor[0] == 'B')
  {
    floor[0] = '-';
  }
  return atoi(floor);
}

void floor_int_to_char(int floor, char *floorChar)
{
  if (floor < 0)
  {
    sprintf(floorChar, "B%d", abs(floor));
  }
  else
  {
    sprintf(floorChar, "%d", floor);
  }
}

void validate_floor_range(int floor)
{
  if (floor < -99)
  {
    printf("Lowest or highest floor cannot be lower than B99\n");
    exit(1);
  }
  else if (floor > 999)
  {
    printf("Lowest or highest floor cannot be higher than 999\n");
    exit(1);
  }
  else if (floor == 0)
  {
    printf("Floor cannot be zero\n");
    exit(1);
  }
}

void compare_highest_lowest(int lowest, int highest)
{
  if (highest < lowest)
  {
    printf("Highest floor (%s%d) cannot be lower than the lowest floor (%s%d)\n",
           (highest < 0) ? "B" : "",
           (highest < 0) ? highest * -1 : highest,
           (lowest < 0) ? "B" : "",
           (lowest < 0) ? lowest * -1 : lowest);
    exit(1);
  }
}