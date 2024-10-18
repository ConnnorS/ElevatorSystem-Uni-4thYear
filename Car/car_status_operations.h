void opening_doors(car_shared_mem *shm_status_ptr);
void open_doors(car_shared_mem *shm_status_ptr);
void closing_doors(car_shared_mem *shm_status_ptr, int *delay_ms);
void close_doors(car_shared_mem *shm_status_ptr);
void set_between(car_shared_mem *shm_status_ptr);
void door_open_close(car_shared_mem *shm_status_ptr, int *delay_ms, int obstruction);