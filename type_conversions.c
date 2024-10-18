#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int check_floor_length(char *floor)
{
  if (strlen(floor) > 4)
  {
    return 0;
  }
  return 1;
}

int check_floor_contents(char *floor)
{
  // start one character over if basement floor
  int index = 0;
  if (floor[0] == 'B')
  {
    index = 1;
  }
  // validate the contents
  for (index = index; index < strlen(floor); index++)
  {
    if (!isdigit(floor[index]))
    {
      return 0;
    }
  }
  return 1;
}

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