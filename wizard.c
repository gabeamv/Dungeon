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
sem_t *wizard_lever;

// handler function for the SIGUSR1 signal from parent process.
void dungeon_signal_handler(int signum) {
    int shift = (int)(dungeon->barrier.spell[0]) % 26; // get the shift value through the ascii of first char
    for (int i = 1; i < SPELL_BUFFER_SIZE + 1; i++) { // iterate through barrier spell
        if (isalpha(dungeon->barrier.spell[i])) { // if alphabetical char
            int asciiDecrypt = (int)(dungeon->barrier.spell[i]) - shift; // ascii of decripted letter with applied shift

            // adjust asciiDecrypt if it is out of range for alphabetical ascii
            if (isupper(dungeon->barrier.spell[i]) && asciiDecrypt < 65) asciiDecrypt = 91 - (65 - asciiDecrypt);
            else if (islower(dungeon->barrier.spell[i]) && asciiDecrypt < 97) asciiDecrypt = 123 - (97 - asciiDecrypt);

            // store the decripted char into wizard spell
            dungeon->wizard.spell[i-1] = asciiDecrypt; 
        }
        
        // if not letter, copy value from barrier spell to wizard spell
        else dungeon->wizard.spell[i-1] = dungeon->barrier.spell[i]; 
    }
}

// handler function for the SIGUSR2 signal from parent process.
void semaphore_signal_handler(int sig) {
    sleep(1); // let other process run
    printf("The wizard holds lever two.\n");
    sem_wait(wizard_lever); // wait to let rogue get treasure
}

// handler function for SIGTERM signal from parent process.
void sigterm_handler(int sig) {
    printf("Unmapping shared memory to dungeon from wizard.\n");
    // unmap from shared memory
    if (munmap(dungeon, sizeof(struct Dungeon)) == -1) {
        printf("Memory has already been unmapped from wizard.\n");
    } 

    printf("Closing shared memory and semaphores in wizard.\n");
    // close shared memory and semaphores
    if (close(fd) == -1) {
        perror("File descriptor failed to close from wizard.\n");
    }

    // close semaphores
    if (sem_close(wizard_lever) == -1) {
        perror("Failed to close wizard semaphore from wizard.\n");
    }
    
    printf("Wizard has terminated.\n");
    exit(0);
}

int main(int argc, char* argv[]) {
    printf("Wizard has been executed.\n");

    // Open shared memory object for read and write.
    struct sigaction dungeon_action; // initialize a sigaction struct to handle DUNGEON_SIGNAL (SIGUSR1)
    dungeon_action.sa_handler = &dungeon_signal_handler; // pointer to the method that will be called when process receives signal.
    if (sigaction(DUNGEON_SIGNAL, &dungeon_action, NULL) == -1) {
        printf("Wizard dungeon signal handling error.\n");
        return 10;
    }

    struct sigaction semaphore_action; // initialize a sigaction struct to handle SEMAPHORE_SIGNAL (SIGUSR2)
    semaphore_action.sa_handler = &semaphore_signal_handler; // pointer to the method that will be called when process receives signal
    if (sigaction(SEMAPHORE_SIGNAL, &semaphore_action, NULL) == -1) {
        printf("Wizard semaphore signal handling error.\n");
        return 10;
    }

    struct sigaction terminate_action; // initialize a sigaction struct to terminate process.
    terminate_action.sa_handler = &sigterm_handler; // pointer to the method that will be called when process receives signal
    if (sigaction(SIGTERM, &terminate_action, NULL) == -1) {
        printf("Wizard terminate signal handling error.\n");
        return 10;
    }

    // unlink if semaphore with this name.
    sem_unlink(dungeon_lever_two);

    // Open semaphore.
    wizard_lever = sem_open(dungeon_lever_two, O_CREAT, 0644, 1);
    if (wizard_lever == SEM_FAILED) {
        perror("Wizard lever in wizard failed.\n");
        return 11;
    }

    // Open shared memory object for read and write.
    fd = shm_open(dungeon_shm_name, O_RDWR, 0666); 
    if (fd == -1) {
        perror("Shared memory failed in wizard.c.\n");
        return 4;
    }

    // Get the starting address of the memory for read and write.
    // Updates to this are visible to other processes which access
    // the same region. Cast it to a Dungeon pointer.
    dungeon = (struct Dungeon*) mmap(NULL, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (dungeon == MAP_FAILED) {
        printf("Mapping shared memory failed in wizard.c.\n");
        return 6;
    }
    
    while (1) { // run process until dungeon is over
        pause();
    }

    return 0;
}