#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ctype.h>
#include <time.h>
#include <semaphore.h>
#include "dungeon_info.h"

int fd; // file descriptor for opened shared memory.
struct Dungeon *dungeon; // where shared memory will be accessed.

// Initialize semaphore.
sem_t *barbarian_lever;

// Initialize semaphore.
sem_t *wizard_lever;

// handler function for the SIGUSR1 signal from parent process
void dungeon_signal_handler(int signum) {
    // stop parent process to initialize max and min angles
    usleep(50000);
    kill(getppid(), SIGSTOP); 
    int min = 0;
    int max = MAX_PICK_ANGLE;
    do {
        dungeon->rogue.pick = (max + min) / 2; // set pick to mid value between possible max and min angles
        dungeon->trap.direction = 't'; // indicate a change has been made
        kill(getppid(), SIGCONT); // continue parent process to see if the right angle is picked.
        usleep(TIME_BETWEEN_ROGUE_TICKS); // sleep this process to let parent process check if angle is correct
        kill(getppid(), SIGSTOP); // stop parent process
        if (!dungeon->trap.locked) break; // break if correct angle is calculated
        // if angle needs to go up, the min increases one above the current pick angle
        else if (dungeon->trap.direction == 'u') min = ((int)(dungeon->rogue.pick)) + 1;
        // if angle needs to go down, the max increases one below the current pick angle
        else if (dungeon->trap.direction == 'd') max = ((int)(dungeon->rogue.pick)) - 1;
    } while (min <= max); // binary search until angle (float) is found.
    kill(getppid(), SIGCONT); // resume the stopped parent process
}

// handler function for the SIGUSR2 signal from parent process.
void semaphore_signal_handler(int sig) {
    sleep(2); // let other process run
    printf("The rogue starts looting the treasure.\n");
    sleep(1); // loot treasure
    for (int i = 0; i < 4; i++) { // iterate through treasure array and copy treasure to spoils
        printf("Rogue gets treasure %d.\n", i + 1);
        dungeon->spoils[i] = dungeon->treasure[i];
        usleep(750000);
    }
    // release both wizard and barbarian semaphores
    printf("Wizard releases lever two.\n");
    sem_post(wizard_lever);
    printf("Barbarian releases lever one.\n");
    sem_post(barbarian_lever);
}

// handler function for SIGTERM signal from parent process.
void sigterm_handler(int sig) {
    printf("Unmapping shared memory to dungeon from rogue.\n");
    // unmap from shared memory
    if (munmap(dungeon, sizeof(struct Dungeon)) == -1) {
        printf("Memory has already been unmapped from rogue.\n");
    } 

    printf("Closing shared memory and semaphores in rogue.\n");
    // close shared memory
    if (close(fd) == -1) {
        perror("File descriptor failed to close from rogue.\n");
    }

    // close semaphores
    if (sem_close(barbarian_lever) == -1) {
        perror("Failed to close barbarian semaphore from rogue.\n");
    }

    if (sem_close(wizard_lever) == -1) {
        perror("Failed to close wizard semaphore from rogue.\n");
    }

    printf("Rogue has terminated.\n");
    exit(0);
}

int main(int argc, char* argv[]) {
    printf("Rogue has been executed.\n");

    struct sigaction dungeon_action; // initialize a sigaction struct to handle DUNGEON_SIGNAL (SIGUSR1)
    dungeon_action.sa_handler = &dungeon_signal_handler; // pointer to the method that will be called when process receives signal
    if (sigaction(DUNGEON_SIGNAL, &dungeon_action, NULL) == -1) {
        printf("Rogue dungeon signal handling error.\n");
        return 10;
    }

    struct sigaction semaphore_action; // initialize a sigaction struct to handle SEMAPHORE_SIGNAL (SIGUSR2)
    semaphore_action.sa_handler = &semaphore_signal_handler; // pointer to the method that will be called when process receives signal
    if (sigaction(SEMAPHORE_SIGNAL, &semaphore_action, NULL) == -1) {
        printf("Rogue semaphore signal handling error.\n");
        return 10;
    }

    struct sigaction terminate_action; // initialize a sigaction struct to terminate process.
    terminate_action.sa_handler = &sigterm_handler; // pointer to the method that will be called when process receives signal
    if (sigaction(SIGTERM, &terminate_action, NULL) == -1) {
        printf("Rogue terminate signal handling error.\n");
        return 10;
    }

    // Open semaphores.
    barbarian_lever = sem_open(dungeon_lever_one, O_CREAT, 0644, 1);
    if (barbarian_lever == SEM_FAILED) {
        perror("Barbarian lever in rogue failed.\n");
        return 11;
    }
    wizard_lever = sem_open(dungeon_lever_two, O_CREAT, 0644, 1);
    if (wizard_lever == SEM_FAILED) {
        perror("Wizard lever in rogue failed.\n");
        return 11;
    }

    // Open shared memory object for read and write.
    fd = shm_open(dungeon_shm_name, O_RDWR, 0666);
    if (fd == -1) {
        perror("Shared memory failed in rogue.c.\n");
        return 4;
    }

    // Get the starting address of the memory for read and write.
    // Updates to this are visible to other processes which access
    // the same region. Cast it to a Dungeon pointer.
    dungeon = (struct Dungeon*) mmap(NULL, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (dungeon == MAP_FAILED) {
        printf("Mapping shared memory failed in rogue.c.\n");
        return 6;
    }

    while (1) { // run process until dungeon is over
        pause();
    }

    return 0;
}