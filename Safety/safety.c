/*
---------- MISRA C Violations Justification ----------
--- using document from Canvas: https://canvas.qut.edu.au/courses/16677/files/4582962?wrap=1 ---

Violation of Rule 20.9 - "The input/output library <stdio.h> shall not be used in production
code."
  - the stdio.h library is required for printf() functions to display error messages to the user.
  - in a real production environment, a safer alternative might be writing to a log file

Violation of Rule 20.8 - "The signal handling facilities of <signal.h> should not be used in production code."
  - the signal handling support is necessary for safe manual project termination
  - as this simulates a real elevator system, the safety system can only terminate with CTRL + C
  - additionally, if the system is terminated with CTRL + C it needs to remain complaint and clean up memory
*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>

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

/* global variables */
car_shared_mem *shm_status_ptr = NULL;
int system_running = 1;

/* function definitions */
int data_consistent(car_shared_mem *shm_ptr);
void exit_cleanup(int signal);
int check_floor_contents(const char *floor);
int check_floor_length(const char *floor);

int main(int argc, char **argv)
{
  /* validate user input */
  if (argc != 2)
  {
    printf("Usage: {car name}\n");
    return 1;
  }

  signal(SIGINT, exit_cleanup);

  /* open the shared memory object */
  char shm_status_name[64];
  snprintf(shm_status_name, sizeof(shm_status_name), "/car%s", argv[1]);

  int shm_status_fd = shm_open(shm_status_name, 2, 0666);
  if (shm_status_fd == -1)
  {
    printf("Unable to access car %s.\n", argv[1]);
    return 1;
  }

  /* map the shared memory */
  shm_status_ptr = mmap(0, sizeof(car_shared_mem), (int)0x2, (int)0x01, shm_status_fd, 0);
  if (shm_status_ptr == MAP_FAILED || shm_status_ptr == NULL)
  {
    perror("mmap failed");
    close(shm_status_fd);
    return 1;
  }

  /* constantly check for updates */
  while (system_running)
  {
    pthread_mutex_lock(&shm_status_ptr->mutex);

    /* wait while ... */
    while (shm_status_ptr->door_obstruction == 0 && // there is no door obstruction
           shm_status_ptr->emergency_stop == 0 &&   // emergency stop hasn't been pressed
           shm_status_ptr->overload == 0 &&         // the overload sensor hasn't been tripped
           data_consistent(shm_status_ptr) &&       // the car's status is valid
           system_running                           // the system is still running
    )
    {
      pthread_cond_wait(&shm_status_ptr->cond, &shm_status_ptr->mutex);
    }

    /* obstruction while doors closing */
    if (shm_status_ptr->door_obstruction == 1 && strcmp((char *)shm_status_ptr->status, "Closing") == 0)
    {
      strncpy(shm_status_ptr->status, "Opening", 8);
    }
    /* emergency stop */
    else if (shm_status_ptr->emergency_stop == 1 && shm_status_ptr->emergency_mode == 0)
    {
      printf("The emergency stop button has been pressed!\n");
      shm_status_ptr->emergency_mode = 1;
    }
    /* overload */
    else if (shm_status_ptr->overload == 1 && shm_status_ptr->emergency_mode == 0)
    {
      printf("The overload sensor has been tripped!\n");
      shm_status_ptr->emergency_mode = 1;
    }
    /* data error */
    else if (!data_consistent(shm_status_ptr))
    {
      printf("Data consistency error!\n");
      shm_status_ptr->emergency_mode = 1;
    }

    pthread_cond_broadcast(&shm_status_ptr->cond);
    pthread_mutex_unlock(&shm_status_ptr->mutex);
  }

  /* un map and close memory */
  if (munmap(shm_status_ptr, sizeof(car_shared_mem)) == -1)
  {
    printf("munmap failed\n");
  }

  close(shm_status_fd);

  return 0;
}

int data_consistent(car_shared_mem *shm_ptr)
{
  /* ignore if emergency mode active */
  if (shm_ptr->emergency_mode == 1)
  {
    return 1;
  }

  /* validate the pointer to shared mem too - throw an error if invalid */
  if (shm_ptr == NULL)
  {
    return 0;
  }

  /* validate status */
  const char *valid_status[5] = {"Open", "Opening", "Closed", "Closing", "Between"};
  int status_is_valid = 0;
  for (int i = 0; i < sizeof(valid_status) / sizeof(valid_status[0]); i++)
  {
    if (strcmp((char *)shm_ptr->status, valid_status[i]) == 0)
    {
      status_is_valid = 1;
      break;
    }
  }

  /* validate the contents of the floor strings */
  int valid_floors = 1;
  /* first check that they don't contain letters */
  if (!check_floor_contents(shm_ptr->current_floor) || !check_floor_contents(shm_ptr->destination_floor))
  {
    valid_floors = 0;
  }

  /* now ensure their length */
  if (!check_floor_length(shm_ptr->current_floor) || !check_floor_length(shm_ptr->destination_floor))
  {
    valid_floors = 0;
  }
  /* validate fields */
  int fields_are_valid = 1;
  if (shm_ptr->open_button > 1 ||
      shm_ptr->close_button > 1 ||
      shm_ptr->door_obstruction > 1 ||
      shm_ptr->overload > 1 ||
      shm_ptr->emergency_stop > 1 ||
      shm_ptr->individual_service_mode > 1 ||
      shm_ptr->emergency_mode > 1)
  {
    fields_are_valid = 0;
  }

  /* validate door obstruction */
  int door_obs_valid = 1;
  if (shm_ptr->door_obstruction == 1 &&
      strcmp((char *)shm_ptr->status, "Opening") != 0 &&
      strcmp((char *)shm_ptr->status, "Closing") != 0)
  {
    door_obs_valid = 0;
  }

  return status_is_valid && fields_are_valid && door_obs_valid && valid_floors;
}

int check_floor_contents(const char *floor)
{
  if (floor == NULL)
  {
    return 0;
  }

  /* start one character over if basement floor */
  int index = 0;
  if (floor[0] == 'B')
  {
    index = 1;
  }

  /* validate the contents of the floor */
  while (floor[index] != '\0')
  {
    if ((unsigned char)floor[index] < '0' || (unsigned char)floor[index] > '9')
    {
      return 0;
    }
    index++;
  }

  return 1;
}

int check_floor_length(const char *floor)
{
  if (floor == NULL)
  {
    return 0;
  }

  int index = 0;
  /* loop through to find the length of the floor - if > 4, invalid length */
  while ((unsigned char)floor[index] != '\0')
  {
    index++;
    if (index > 4)
    {
      return 0;
    }
  }

  return 1;
}

void exit_cleanup(int signal)
{
  /* stop the while loop from running */
  system_running = 0;
  pthread_mutex_lock(&shm_status_ptr->mutex);
  pthread_cond_signal(&shm_status_ptr->cond);
  pthread_mutex_unlock(&shm_status_ptr->mutex);
}