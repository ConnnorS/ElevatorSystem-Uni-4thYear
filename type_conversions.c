#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int floor_char_to_int(char *floor)
{
  char temp[8];
  strcpy(temp, floor);
  if (temp[0] == 'B')
  {
    temp[0] = '-';
  }
  return atoi(temp);
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

int validate_floor_range(int floor)
{
  if (floor < -99 || floor > 999 || floor == 0)
  {
    return -1;
  }
  return 1;
}