# Define the compiler and flags
CC = gcc
FLAGS = -lpthread -Wall -Wpedantic -Wextra -Wshadow -Wformat=2 -Wconversion -Wnull-dereference
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
INTERNAL_SRC = ./Internal/internal.c type_conversions.c
internal: $(INTERNAL_SRC)
	$(CC) -o internal $(INTERNAL_SRC) $(FLAGS)

# ---- SAFETY COMPONENT ---- #
SAFETY_SRC = ./Safety/safety.c
safety: $(SAFETY_SRC)
	$(CC) -o safety $(SAFETY_SRC) $(FLAGS)

# ---- MOVES INTO TEST FOLDER FOR TESTING ---- #
for_testing:
	make
	mv $(FILES) ./test
	make -C /home/cab403/CAB403-A2---Major-Project/test

# ---- CLEAN EVERYTHING ---- #
clean:
	rm -f $(FILES)
	rm -f /home/cab403/CAB403-A2---Major-Project/test/car \
	      /home/cab403/CAB403-A2---Major-Project/test/call \
	      /home/cab403/CAB403-A2---Major-Project/test/controller \
	      /home/cab403/CAB403-A2---Major-Project/test/internal \
	      /home/cab403/CAB403-A2---Major-Project/test/safety	
	make -C /home/cab403/CAB403-A2---Major-Project/test clean
