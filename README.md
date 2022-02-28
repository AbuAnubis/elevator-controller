# elevator-controller
## Description
This program is written in a way that allows its use in embedded applications:
- all used libraries are in the C99 standard
- compiler-dependent variable types (like `int`) are avoided to improve program's architecture independence
- variable sizes are strictly set, to preserve memory - one Elevator struct needs 22B + pointer size, Floor struct needs 15B
- data type sizes and config variables are easily changeable to facilitate tailoring the application to use environment

## Installation and executing
Compile `engine.c` and run the compiled executable. For example on Linux:
```
gcc engine.c -o engine
./engine
```

## Commands
All commands are case-insensitive. Unrecognised commands will be ignored. Commands with invalid arguments will be ignored.
### Help
lists available commands
```
? [or "h"]
```

### Step
makes one step in the simulation
```
s
```

### Print
prints current elevators state
```
p
```

### Call
calls an elevator from a particular floor (like pressing a button on that floor)
```
c <floor> <direction>
```
Arguments:
- `floor` - floor where the elevator is being called
- `direction` - `+` or a positive number to go upwards; `-` or a negative number to go downwards; anything other will be ignored.

### Button
presses one of the buttons inside the elevator
```
b <elevator> <floor>
```
Arguments:
- `elevator` - number of the elevator where the button is being pressed
- `floor` - floor number corresponding to the button being pressed

### Exit
exits the program
```
e
```

### Combining commands
commands can be combined and executed simultaneously. Examples:
- `sp` will make a step and print elevators status
- `ssss` will make four steps
- `c 4 + c 7 -` will make two separate calls (4 up and 7 down)
- `c 5 - ss S b 3 4 s p` will make a call (5 down), make 3 steps, press 4th button in elevator #3, make a step and print

## Algorithm
Upon calling an elevator from a floor, the floor calculates metric for each elevator available and chooses one with the lowest metric. 

Elevator going in one direction (up or down) will stop at every floor on its way that either has a corresponding button inside pressed or has someone waiting for that elevator and wants to go in the same direction as the elevator is going. After reaching its final destination elevator goes to a floor that is waiting for that elevator (if there are more than one such floors, the earliest request is taken). 

Metric is calculated based on:
- Distance from the elevator to the floor
- Current elevator movement
- Other floors waiting for that elevator
Metric is just a method of choosing best elevators to request - it might come either sooner (if the elevator stops on this floor while serving requests from much earlier) or later (it's impossible to predict what buttons passengers will press inside).

## Configuration
Preprocessor constants in the beginning of `engine.c` can be altered, depending on needs:
- `NFLOOR` - number of floors in the building
- `NELEV` - number of elevators in the system
- `TFLOOR` - how many ticks it takes for elevator to move one floor in the simulation.
- `TSTOP` - how many ticks elevator stops for in the simulation.
