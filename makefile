# Define the compiler and flags
CC = gcc
FLAGS = -lpthread -Wall
FILES = car call controller internal safety

# ---- ALL ---- #
all:
	make $(FILES)

# ---- CAR COMPONENT ---- #
CAR_SRC = ./Car/car.c ./Car/car_helpers.c common_comms.c type_conversions.c ./Car/car_status_operations.c
car: $(CAR_SRC)
	$(CC) -o car $(CAR_SRC) $(FLAGS)

# ---- CONTROLLER COMPONENT ---- #
CONTROLLER_SRC = ./Controller/controller.c ./Controller/controller_helpers.c common_comms.c type_conversions.c
controller: $(CONTROLLER_SRC)
	$(CC) -o controller $(CONTROLLER_SRC) $(FLAGS)

# ---- CALL COMPONENT ---- #
CALL_SRC = ./Call/call.c common_comms.c type_conversions.c
call: $(CALL_SRC)
	$(CC) -o call $(CALL_SRC) $(FLAGS)

# ---- INTERNAL COMPONENT ---- #
INTERNAL_SRC = ./Internal/internal.c ./Internal/internal_helpers.c type_conversions.c
internal: $(INTERNAL_SRC)
	$(CC) -o internal $(INTERNAL_SRC) $(FLAGS)

# --- SAFETY COMPONENT ---- #
SAFETY_SRC = ./Safety/safety.c
safety: $(SAFETY_SRC)
	$(CC) -o safety $(INTERNAL_SRC) $(FLAGS)

clean:
	rm -f $(FILES)
