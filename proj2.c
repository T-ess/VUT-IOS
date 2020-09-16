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
#include <stdbool.h>

#include <limits.h>

#define MMAP(pointer) {(pointer) = mmap(NULL, sizeof(*(pointer)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);}
#define UNMAP(pointer) {munmap((pointer), sizeof((pointer)));}
#define fork_err "An error occurred while trying to fork the process!\n"
#define argv_err "Wrong arguments!\n"
#define imm_err "An error occurred in the immigrant function!\n"

FILE *out_file;

sem_t *noJudge = NULL;
sem_t *mutex = NULL;
sem_t *confirmed = NULL;
sem_t *Jexit = NULL;
sem_t *allGone = NULL;
sem_t *allSignedIn = NULL;
int *entered = NULL;
int *checked = NULL;
int *inBuilding = NULL;
int *processNum = NULL;
int *immNum = NULL;
int *judge = NULL;
int *totalIssued = NULL;

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
        return 1;
    }

    // semaphores
    if ((noJudge = sem_open("/xburia28.ios.proj2.noJudge", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) return -1;
    if ((mutex = sem_open("/xburia28.ios.proj2.mutex", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) return -1;
    if ((confirmed = sem_open("/xburia28.ios.proj2.confirmed", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) return -1;
    if ((Jexit = sem_open("/xburia28.ios.proj2.Jexit", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) return -1;
    if ((allGone = sem_open("/xburia28.ios.proj2.allGone", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) return -1;
    if ((allSignedIn = sem_open("/xburia28.ios.proj2.allSignedIn", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) return -1;

    // map shared variables
    MMAP(entered);
    MMAP(checked);
    MMAP(processNum);
    MMAP(inBuilding);
    MMAP(immNum);
    MMAP(judge);
    MMAP(allSignedIn);
    MMAP(totalIssued);

    return 0;
}

int processArguments(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "5 arguments are needed to run the program!\n");
        return 1;
    }

    char *err;
    if((arg.immAmount = (int)strtol(argv[1], &err, 10)) <= 0) {
        fprintf(stderr, argv_err);
        return 1;
    } else if (arg.immAmount < 1 || *err != '\0') {
        fprintf(stderr, argv_err);
        return 1;
    }

    if((arg.time_newImm = (int)strtol(argv[2], &err, 10)) <= 0) {
        fprintf(stderr, argv_err);
        return 1;
    } else if (arg.time_newImm < 0 || arg.time_newImm > 2000 || *err != '\0') {
        fprintf(stderr, argv_err);
        return 1;
    }

    if((arg.time_judgeOutside = (int)strtol(argv[3], &err, 10)) <= 0) {
        fprintf(stderr, argv_err);
        return 1;
    } else if (arg.time_judgeOutside < 0 || arg.time_judgeOutside > 2000 || *err != '\0') {
        fprintf(stderr, argv_err);
        return 1;
    }

    if((arg.time_getCertificate = (int)strtol(argv[4], &err, 10)) <= 0) {
        fprintf(stderr, argv_err);
        return 1;
    } else if (arg.time_getCertificate < 0 || arg.time_getCertificate > 2000 || *err != '\0') {
        fprintf(stderr, argv_err);
        return 1;
    }

    if((arg.time_issueCertificate = (int)strtol(argv[5], &err, 10)) <= 0) {
        fprintf(stderr, argv_err);
        return 1;
    } else if (arg.time_issueCertificate < 0 || arg.time_issueCertificate > 2000 || *err != '\0') {
        fprintf(stderr, argv_err);
        return 1;
    }
    return 0;
}

void clean() {
    // unmap shared variables
    UNMAP(entered);
    UNMAP(checked);
    UNMAP(processNum);
    UNMAP(immNum);
    UNMAP(inBuilding);
    UNMAP(allSignedIn);
    UNMAP(judge);
    UNMAP(totalIssued);

    // close semaphores
    sem_close(noJudge);
    sem_close(mutex);
    sem_close(confirmed);
    sem_close(Jexit);
    sem_close(allGone);
    sem_close(allSignedIn);

    // unlink semaphores
    sem_unlink("xburia28.ios.proj2.noJudge");
    sem_unlink("xburia28.ios.proj2.mutex");
    sem_unlink("xburia28.ios.proj2.confirmed");
    sem_unlink("xburia28.ios.proj2.Jexit");
    sem_unlink("xburia28.ios.proj2.allGone");
    sem_unlink("xburia28.ios.proj2.allSignedIn");

    // close file
    if (out_file != NULL) fclose(out_file);
    return;
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

int judgeProcess() {
    while (true) {
        msleep(randomNum(arg.time_judgeOutside));
        sem_wait(noJudge);
        sem_wait(mutex);
        fprintf(stdout, "%d : JUDGE : wants to enter.\n", ++(*processNum));
        fprintf(stdout, "%d : JUDGE : enters : %d : %d : %d\n", ++(*processNum), *entered, *checked, *inBuilding);
        if (*entered > *checked) {
            fprintf(stdout, "%d : JUDGE : waits for imm : %d : %d : %d\n", ++(*processNum), *entered, *checked, *inBuilding);
            sem_post(mutex);
            sem_wait(allSignedIn);
        }
        fprintf(stdout, "%d : JUDGE : starts confirmation : %d : %d : %d\n", ++(*processNum), *entered, *checked, *inBuilding);
        msleep(randomNum(arg.time_getCertificate));
        *totalIssued += *checked;
        *checked = 0;
        *entered = 0;
        fprintf(stdout, "%d : JUDGE : ends confirmation : %d : %d : %d\n", ++(*processNum), *entered, *checked, *inBuilding);
        msleep(randomNum(arg.time_getCertificate));
        fprintf(stdout, "%d : JUDGE : leaves : %d : %d : %d\n", ++(*processNum), *entered, *checked, *inBuilding);
        fprintf(stdout, "%d", *totalIssued);
        if (*totalIssued >= arg.immAmount) {
            fprintf(stdout, "%d : JUDGE : finishes.\n", ++(*processNum));
            exit(0);
        }
    }
}

int immigrant() {
    int immID = (*immNum)++;
    sem_wait(noJudge);
    fprintf(stdout, "%d : IMM %d : enters : %d : %d : %d\n", ++(*processNum), immID, ++(*entered), *checked, ++(*inBuilding));
    sem_post(noJudge);

    sem_wait(mutex);
    fprintf(stdout, "%d : IMM %d : checks : %d : %d : %d\n", ++(*processNum), immID, *entered, ++(*checked), *inBuilding);
    if ((*judge == 1) && (*entered == *checked)) {
        sem_post(allSignedIn);
    } else {
        sem_post(mutex);
    }
    sem_wait(confirmed);
    fprintf(stdout, "%d : IMM %d : wants certificate : %d : %d : %d\n", ++(*processNum), immID, *entered, *checked, *inBuilding);
    msleep(randomNum(arg.time_getCertificate));
    fprintf(stdout, "%d : IMM %d : got certificate : %d : %d : %d\n", ++(*processNum), immID, *entered, *checked, *inBuilding);
    sem_wait(Jexit);
    fprintf(stdout, "%d : IMM %d : leaves : %d : %d : %d\n", ++(*processNum), immID, *entered, *checked, (*inBuilding)--);
    if (*checked == 0) {
        sem_post(allGone);
    } else {
        sem_post(Jexit);
    }
    exit(0);
}

int immGenerator() {
    for(int i = 1; i <= arg.immAmount; i++) {
        pid_t imm = fork();
        if (imm < 0) {
            fprintf(stderr, fork_err);
            clean();
            exit(-1);
        } else if (imm == 0) {
            // process immigrant
            immigrant();
        }
        msleep(randomNum(arg.time_newImm));
    }
    int status;
    wait(&status);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (initialize() == 1) {
        clean();
        return 1;
    }

    if (processArguments(argc, argv) != 0) {
        clean();
        return 1;
    }

    pid_t judge = fork();
    if (judge < 0) {
        fprintf(stderr, fork_err);
        clean();
        return -1;
    } else if (judge == 0) {
        // process judge
        judgeProcess();
    } else {
        pid_t generator = fork();
        if (generator < 0) {
            fprintf(stderr, fork_err);
            clean();
            return -1;
        } else if (generator == 0) {
            // generate immigrants
            immGenerator();
        }
    }
    int status;
    wait(&status);
    wait(&status);
    clean();
    return 0;
}
