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
#include <errno.h>

#define MMAP(pointer) {(pointer) = mmap(NULL, sizeof(*(pointer)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);}
#define UNMAP(pointer) {munmap((pointer), sizeof((pointer)));}
#define fork_err "An error occurred while trying to fork the process!\n"
#define argv_err "Wrong arguments!\n"

FILE *out_file;

struct ARG{
    int immAmount;
    int time_newImm;
    int time_judgeOutside;
    int time_getCertificate;
    int time_issueCertificate;
} arg;

int initialize() {
    if((out_file = fopen("proj2.out", "w")) == NULL) {
        fprintf(stderr, "Error: the output file could not be opened!\n");
        exit(-1);
    }
    // sdilene promenne
    return 0;
}

int processArguments(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "5 arguments are needed to run the program!\n");
        exit(-2);
    }

    char *err;
    if((arg.immAmount = strtol(argv[1], &err, 10)) <= 0) {
        fprintf(stderr, argv_err);
        exit(-3);
    } else if (arg.immAmount < 1 || *err != '\0') {
        fprintf(stderr, argv_err);
        exit(-3);
    }

    if((arg.time_newImm = strtol(argv[2], &err, 10)) <= 0) {
        fprintf(stderr, argv_err);
        exit(-3);
    } else if (arg.time_newImm < 0 || arg.time_newImm > 2000 || *err != '\0') {
        fprintf(stderr, argv_err);
        exit(-3);
    }

    if((arg.time_judgeOutside = strtol(argv[3], &err, 10)) <= 0) {
        fprintf(stderr, argv_err);
        exit(-3);
    } else if (arg.time_judgeOutside < 0 || arg.time_judgeOutside > 2000 || *err != '\0') {
        fprintf(stderr, argv_err);
        exit(-3);
    }

    if((arg.time_getCertificate = strtol(argv[4], &err, 10)) <= 0) {
        fprintf(stderr, argv_err);
        exit(-3);
    } else if (arg.time_getCertificate < 0 || arg.time_getCertificate > 2000 || *err != '\0') {
        fprintf(stderr, argv_err);
        exit(-3);
    }

    if((arg.time_issueCertificate = strtol(argv[5], &err, 10)) <= 0) {
        fprintf(stderr, argv_err);
        exit(-3);
    } else if (arg.time_issueCertificate < 0 || arg.time_issueCertificate > 2000 || *err != '\0') {
        fprintf(stderr, argv_err);
        exit(-3);
    }
}

void clean() {
    // unmap sdilene promenne
    // sem_close
    // sem_unlink
    if (out_file != NULL) fclose(out_file);
}

int randomNum(int limit) {
    int num = rand() % (limit + 1);
    return num;
}

int msleep(long time) {
    struct timespec ts;
    int res;

    if (time < 0) {
        errno = EINVAL;
        return -1;
    }
    ts.tv_sec = time / 1000;
    ts.tv_nsec = (time % 1000) * 1000000;
    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);
    return res;
}

int judge() {

}

int immigrant() {

}

void immGenerator() {
    for(int i = 1; i <= arg.immAmount; i++) {
        fprintf(stdout, "inside generator\n");
        pid_t imm = fork();
        if (imm < 0) {
            fprintf(stderr, fork_err);
            exit(-1);
        } else if(imm == 0) {
            // process immigrant
            fprintf(stdout, "ahoj ja jsem imigrant %d \n", i);
            exit(0);
        }
        msleep(randomNum(arg.time_newImm));
    }
    int status;
    wait(&status);
    exit(0);
}

int main(int argc, char *argv[]) {
    initialize();
    processArguments(argc, argv);

    pid_t judge = fork();
    if (judge < 0) {
        fprintf(stderr, fork_err);
        exit(-1);
    } else if (judge == 0) {
        // process judge
        fprintf(stdout, "judge\n");
        exit(0);
    } else {
        pid_t generator = fork();
        if (generator < 0) {
            fprintf(stderr, fork_err);
            exit(-1);
        } else if (generator == 0) {
            // generate immigrants
            fprintf(stdout, "generator\n");
            immGenerator();
        }
    }
    int status;
    wait(&status);
    wait(&status);
    clean();
    exit(0);
}
