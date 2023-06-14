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

int main(int argc, char* argv[]) {
    pid_t barbarian, wizard, rogue; // pids of processes
    int barbarianStatus, wizardStatus, rogueStatus; // exit status

    // unlink if there is memory with this name
    shm_unlink(dungeon_shm_name);

    // Create shared memory object for read and write.
    int fd = shm_open(dungeon_shm_name, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("Shared memory failed in game.c.\n");
        return 4;
    }

    // Set size of the shared memory to the size of the dungeon.
    if (ftruncate(fd, sizeof(struct Dungeon)) == -1) {
        printf("Setting size of shared memory failed in game.c.\n");
        return 5;
    } 

    // Get the starting address of the memory for read and write.
    // Updates to this are visible to other processes which access
    // the same region. Cast it to a Dungeon pointer.
    struct Dungeon *dungeon = (struct Dungeon*) mmap(NULL, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (dungeon == MAP_FAILED) {
        printf("Mapping shared memory failed in game.c.\n");
        return 6;
    }

    barbarian = fork(); // get pid of barbarian
    if (barbarian < 0) {
        printf("Barbarian fork failed.\n");
        return 1;
    }

    else if (barbarian == 0) { // barbarian process
        // Execute barbarian executable.
        int exeBarbarian = execl("./barbarian", "./barbarian", NULL);
        if (exeBarbarian == -1) {
            printf("Barbarian execution failed.\n");
            return 7;
        }
    }

    else { // parent process
        usleep(500000); // let barbarian execute (0.5 seconds)
        wizard = fork(); // get pid of wizard
        if (wizard < 0) {
            printf("Wizard fork failed.\n");
            return 2;
        }

        else if (wizard == 0) { // wizard process
            // Execute wizard executable
            int exeWizard = execl("./wizard", "./wizard", NULL);
            if (exeWizard == -1) {
                printf("Wizard execution failed.\n");
                return 8;
            }

        }

        else { // parent process
            sleep(1); // let wizard execute (1 second)
            rogue = fork(); // get pid of rogue
            if (rogue < 0) {
                printf("Rogue fork failed.\n");
                return 3;
            }

            else if (rogue == 0) { // rogue process running
                // Execute rogue executable.
                int exeRogue = execl("./rogue", "./rogue", NULL);
                if (exeRogue == -1) {
                    printf("Rogue execution failed.\n");
                    return 9;
                }
            }

            else { // parent process
                usleep(1500000); // let rogue execute (1.5 seconds)
                RunDungeon(wizard, rogue, barbarian); // run the dungeon
                sleep(1);

                if (!dungeon->running) printf("Exited from the dungeon.\n");

                // terminate all child processes gracefully through SIGTERM
                usleep(250000);
                kill(rogue, SIGTERM);
                usleep(250000);
                kill(wizard, SIGTERM);
                usleep(250000);
                kill(barbarian, SIGTERM);
                usleep(250000);

                printf("Unmapping shared memory to dungeon from game.c.\n");
                // unmap from shared memory
                if (munmap(dungeon, sizeof(struct Dungeon)) == -1) {
                    printf("Memory has already been unmapped from game.c.\n");
                }
                // close file descriptor of shared memory
                if (close(fd) == -1) {
                    perror("Failed to close file descriptor from game.c.");
                }
                 
                printf("Unlinking shared memory and all semaphores.\n");
                // unlink shared memory
                if (shm_unlink(dungeon_shm_name) == -1) {
                    printf("Shared memory has already been unlinked.\n");
                }
                
                // unlink dungeon_level_one semaphore
                if (sem_unlink(dungeon_lever_one) == -1) {
                    perror("sem_unlink\n");
                }
                // unlink dungeon_level_two semaphore
                if (sem_unlink(dungeon_lever_two) == -1) {
                    perror("sem_unlink\n");
                }
                printf("Game has terminated.\n");
            }
        }
    }
    return 0;
}