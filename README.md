# Mutlithreaded-Parallel-15-Puzzle-Solver
Parallelized 15-Puzzle solver using POSIX Threads


## Execution Details:
#### Step 1: Specify the initial position in the text file “input.txt”.
Input Format: It is written as a grid of 4x4 grid of integers from 0 to 15(Black space is specified as 0). For example,
```
1 2 4 8
5 0 6 3
9 10 7 12
13 14 11 15
```
#### Step 2: Compile and run the .c files.
Either directly run ‘make’ which compiles both the porgrams into two separate executables ‘puzzle_a.out’, and ‘puzzle_b.out’ and run them as indicated below, or compile separately as mentioned below.
1. For puzzle_a:
    1. `gcc –g puzzle_a.c –o puzzle_a –lpthread`
    2. `./puzzle_a <maximum_number_of_threads>`
2. For puzzle_b: 
    1. `gcc –g puzzle_b.c –o puzzle_b –lpthread`
    2. `./puzzle_b <maximum_number_of_threads>`	

*(NOTE: `<maximum_number_of_threads>` is a command line argument passed. It should be kept below MAX_THREADS defined as macro in the respective .c files)*
#### Step 3: Interpretation of the output.
The output contains the following:
3.1. Solution: It specifies the sequence of moves required to reach the desired orientation. It is a string consisting of characters, ‘L’, ‘R’, ‘U’, ‘D’ signifying that the blank element should be exchanged with the block present on its left, right, up, down respectively.
3.2. Time taken: It signifies the time required by the program in finding the solution.
(NOTE: Solution may be different on different runs of the programs, as the path taken by the algorithm is dependent on spawning of the threads)

