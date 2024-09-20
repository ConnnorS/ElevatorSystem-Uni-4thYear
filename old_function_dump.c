void print_car_shared_mem(const car_shared_mem *shared_mem, const char *name)
{
  if (shared_mem == NULL)
  {
    printf("Shared memory pointer is NULL\n");
    return;
  }

  // Since `pthread_mutex_t` and `pthread_cond_t` are not printable, we'll skip them
  printf("SHARED MEM INFO:\n");
  printf("Shared mem name: %s\n", name);
  printf("Current Floor: %s\n", shared_mem->current_floor);
  printf("Destination Floor: %s\n", shared_mem->destination_floor);
  printf("Status: %s\n", shared_mem->status);
  printf("Open Button: %d\n", shared_mem->open_button);
  printf("Close Button: %d\n", shared_mem->close_button);
  printf("Door Obstruction: %d\n", shared_mem->door_obstruction);
  printf("Overload: %d\n", shared_mem->overload);
  printf("Emergency Stop: %d\n", shared_mem->emergency_stop);
  printf("Individual Service Mode: %d\n", shared_mem->individual_service_mode);
  printf("Emergency Mode: %d\n", shared_mem->emergency_mode);
  printf("\n");
}

void print_car_info(const car_info *car)
{
  printf("CAR INFO:\n");
  printf("Car Name: %s\n", car->name);
  printf("Lowest Floor: %s\n", car->lowest_floor);
  printf("Highest Floor: %s\n", car->highest_floor);
  printf("Delay (ms): %d\n", car->delay_ms);
  printf("\n");
}