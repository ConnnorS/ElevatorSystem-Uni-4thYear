void opening_doors(car_shared_mem *shm_status_ptr);
void open_doors(car_shared_mem *shm_status_ptr);
void closing_doors(car_shared_mem *shm_status_ptr);
void close_doors(car_shared_mem *shm_status_ptr, int *delay_ms);
void set_between(car_shared_mem *shm_status_ptr);
void get_status(car_shared_mem *shm_status_ptr, char *status);
void handle_obstruction(car_shared_mem *shm_status_ptr, int *delay_ms);