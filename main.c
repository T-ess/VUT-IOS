#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <semaphore.h>
#include <fcntl.h> 
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define MMAP(pointer) {(pointer) = mmap(NULL, sizeof(*(pointer)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);}
#define UNMAP(pointer) {munmap((pointer), sizeof((pointer)));}

FILE *out_file;

int initialize() {
    if((out_file = fopen("proj2.out", "w")) == NULL) {
        fprintf(stderr, "Error: the output file could not be opened!\n");
        exit(-1);
    }
    // sdilene promenne
    return 0;
}

void clean() {
    // unmap sdilene promenne
    // sem_close
    // sem_unlink
    if (out_file != NULL) fclose(out_file);
}

int randomNum(int limit) {
    int i;
    int num = rand() % (limit + 1);
    return num;
}

void judge() {

}

void immGenerator(int amount) {
    for(int i = 1; i <= amount; i++) {
        fprintf(stdout, "inside generator\n");
        pid_t imm = fork();
        if(imm == 0) {
            // process immigrant
            fprintf(stdout, "ahoj ja jsem imigrant %d \n", i);
            exit(0);
        }
        sleep(2);
    }
    exit(0);
}

int main() {
    initialize();
    int immigrants = 6;

    pid_t judge = fork();
    if (judge == 0) {
        // process judge
        fprintf(stdout, "judge\n");
        exit(0);
    } else {
        pid_t generator = fork();
        if (generator == 0) {
            // generate immigrants
            fprintf(stdout, "generator\n");
            immGenerator(immigrants);
        }
    }
    int status;
    wait(&status);
    wait(&status);
    clean();
    exit(0);
    return 0;
}
