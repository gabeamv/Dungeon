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
#include <semaphore.h>
#include "dungeon_info.h"


int fd; // file descriptor for opened shared memory.
struct Dungeon *dungeon; // where shared memory will be accessed.

// Initialize semaphore.
sem_t *barbarian_lever;

// handler function for the SIGUSR1 signal from parent process.
void dungeon_signal_handler(int sig) {
    dungeon->barbarian.attack = dungeon->enemy.health; // copy enemy health into barbarian attack field
}

// handler function for the SIGUSR2 signal from parent process.
void semaphore_signal_handler(int sig) {
    printf("The barbarian holds lever one.\n");
    sem_wait(barbarian_lever); // wait to let rogue get treasure
}

// handler function for SIGTERM signal from parent process.
void sigterm_handler(int sig) {
    printf("Unmapping shared memory to dungeon from barbarian.\n");
    // unmap from shared memory
    if (munmap(dungeon, sizeof(struct Dungeon)) == -1) {
        printf("Memory has already been unmapped from barbarian.\n");
    } 

    printf("Closing shared memory and semaphores in barbarian.\n");
    // close shared memory and semaphores
    if (close(fd) == -1) {
        perror("File descriptor failed to close from barbarian.\n");
    }

    // close semaphores
    if (sem_close(barbarian_lever) == -1) {
        perror("Failed to close barbarian semaphore from barbarian.\n");
    }
    
    printf("Barbarian has terminated.\n");
    exit(0);
}

int main(int argc, char* argv[]) {
    printf("Barbarian has been executed.\n"); // output process has been executed
    
    struct sigaction dungeon_action; // initialize a sigaction struct to handle DUNGEON_SIGNAL (SIGUSR1)
    dungeon_action.sa_handler = &dungeon_signal_handler; // pointer to the method that will be called when process receives signal
    if (sigaction(DUNGEON_SIGNAL, &dungeon_action, NULL) == -1) {
        printf("Barbarian dungeon signal handling error.\n");
        return 10;
    }

    struct sigaction semaphore_action; // initialize a sigaction struct to handle SEMAPHORE_SIGNAL (SIGUSR2)
    semaphore_action.sa_handler = &semaphore_signal_handler; // pointer to the method that will be called when process receives signal
    if (sigaction(SEMAPHORE_SIGNAL, &semaphore_action, NULL) == -1) {
        printf("Barbarian semaphore signal handling error.\n");
        return 10;
    }

    struct sigaction terminate_action; // initialize a sigaction struct to terminate process.
    terminate_action.sa_handler = &sigterm_handler; // pointer to the method that will be called when process receives signal
    if (sigaction(SIGTERM, &terminate_action, NULL) == -1) {
        printf("Barbarian terminate signal handling error.\n");
        return 10;
    }

    // unlink if semaphore with this name
    sem_unlink(dungeon_lever_one);

    // Open semaphore.
    barbarian_lever = sem_open(dungeon_lever_one, O_CREAT, 0644, 1);
    if (barbarian_lever == SEM_FAILED) {
        perror("Barbarian lever in barbarian failed.\n");
        return 11;
    }

    // Open shared memory object for read and write.
    fd = shm_open(dungeon_shm_name, O_RDWR, 0666);

    // Get the starting address of the memory for read and write.
    // Updates to this are visible to other processes which access
    // the same region. Cast it to a Dungeon pointer.
    dungeon = (struct Dungeon*) mmap(NULL, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (dungeon == MAP_FAILED) {
        printf("Mapping shared memory failed in barbarian.c.\n");
        return 6;
    }

    while (1) { // run process until dungeon is over
        pause();
    }

    return 0;
}