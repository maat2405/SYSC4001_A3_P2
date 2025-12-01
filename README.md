# Assignment 3 Part 2 
This project simulates multiple Teaching Assistants marking student exams at the same time. It demonstrates shared memory, semaphores, and synchronization between processes while safely updating a shared rubric and grading multiple exam files.

# How to Compile and Run
1. Confirm your rubric file is in the project directory.
2. Confirm exam files are present, there must be multiple exam files inside the exams folder
3. Confirm there is a terminating exam. Ensure there is an exam for student 9999, this signals the stop.
4. Open a Linux or macOS terminal. Navigate to the project directory:
   cd /path/to/SYSC4001_A3_P2
5. Compile part A:
   g++ -std=c++17 -pthread ta_part_a.cpp -o ta_part_a
6. Compile part B:
   g++ -std=c++17 -pthread ta_part_b.cpp -o ta_part_b
7. Run part A, make sure # is replaced with number of TA processes (greater than 2)
   ./ta_part_a # ./exams rubric.txt
8. Run part B, same as above replace # with number of TAs
   ./ta_part_b # ./exams rubric.txt
9. Program runs and prints TA actions to the terminal

# Authors
Sonai Haghgooie 101306866
Maathusan Sathiendran 101302780
