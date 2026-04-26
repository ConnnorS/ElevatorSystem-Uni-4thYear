# Simulated Elevator System Written in C
This project is an elevator system written in C, developed as part of my final year CAB403 coursework. It is a simulated system which aims to behave and operate as if it was in a real elevator system environment, handling floor inputs from users, opening and closing doors, efficiently adding floors to its queue that are on its way, emergency management, and more.

# Components
## Call
A simple component which simulates a call pad which would be sitting outside the elevator doors on every floor of the building. When a user is on a given floor they would select the floor that they would like to go to and this component sends that message to the *Controller*. It validates input, ensures the floor numbers are correctly formatted, connects to the controller via an internal TCP connection, and relays the source and destination floors.

## Car
Represents an elevator car that passengers can travel in and moves between floors. In the elevator system, each *Car* is spun up onto its own thread to simulate a real elevator system where multiple cars would be operating and moving independently and asynchronously. Each car has four attributes:

- **Car Name** - the name given to the car to identify it (must be unique in the context of the whole elevator system)
- **Lowest Floor** - the lowest floor that the car can travel to
- **Highest Floor** - the highest floor that the car can travel to
- **Delay** - how long the car is to take to do a certain thing, like opening or closing doors, or travelling a single floor. This is mostly for assessment purposes.

Like the *Call* component, each *Car* also communicates with the *Controller*, receiving commands and sending status messages at a given interval. Each *Car* spins up its own thread which sends status messages to the *Controller* every *{delay}* milliseconds and this thread spins up its own thread for constantly watching for messages from the *Controller* and acting accordingly.

The *Car* component also uses a shared memory object for each of its threads to use and interact with. The shared memory stores data like the current floor the car is on, the destination floor the car is expected to be at, the open/opening/close/closing status of the doors, and more. This shared memory is protected by a mutex so that race conditions and other undefined behaviour do not occur.

## Controller
Perhaps the most complex and advanced component of the system, the *Controller* controls all aspects of the elevator system, tracking cars, sending instructions, receiving calls from passengers, and much more. 

On startup, the *Controller* creates its own TCP server which the system components connect to and send and recieve messages. When a new client connects (like a *Car* or *Call* pad), a new thread is created by the *Controller* for the sole purpose of handling that connection. This ensures that all the system's components can be handled and communicated with asynchronously and that waiting on a specific component does not block the rest of the system. It stores all client information in a mutex-protected array of pointers to `client_t` structs so that every thread on the *Controller* can access information about other threads to better allocate floors or handle messages (for example, the *Controller* might receive a message to go from floor 5 to floor 10, it notices that *Car* 2 is already going from floor 2 to floor 11, so it tells *Car* 2 to stop at floor 5 & 10 on its way).

## Internal
Each *Car* has its own *Internal* system, running on its own thread, to handle button presses inside the car, changes in current or destination floors, or emergency modes. This simulates a control panel you often see inside real elevator cars. When certain buttons are pressed (for example, the door open button) the *Internal* system will update the *Car*'s shared memory object with a new status or information and allow the car to act accordingly.

## Safety
Each *Car* also has its own *Safety* system which aims to be a MISRA-C compliant, safety-critical system to handle any safety or emergency issues with the *Car*. The *Safety* system handles things like door obstructions when closing, emergency stop, emergency mode, and other input validation. The goal of this component was to comply with the MISRA-C standards and any violations of those standards were documented at the top of the file.