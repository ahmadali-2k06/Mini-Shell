#include "sync_demo.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>

#define NUM_WORKERS 10
#define INCREMENTS_PER_WORKER 1000
#define EXPECTED_TOTAL (NUM_WORKERS * INCREMENTS_PER_WORKER)

// Union required for semctl
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

int create_shared_memory(void) {
    int shmid = shmget(IPC_PRIVATE, sizeof(shared_data_t), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("minishell: Failed to create shared memory");
    }
    return shmid;
}

void cleanup_shared_memory(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) < 0) {
        perror("minishell: Failed to remove shared memory");
    }
}

int create_semaphore(void) {
    int semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (semid < 0) {
        perror("minishell: Failed to create semaphore");
        return -1;
    }

    union semun arg;
    arg.val = 1; // Binary semaphore initialized to 1
    if (semctl(semid, 0, SETVAL, arg) < 0) {
        perror("minishell: Failed to initialize semaphore");
        semctl(semid, 0, IPC_RMID); // cleanup
        return -1;
    }

    return semid;
}

void cleanup_semaphore(int semid) {
    if (semctl(semid, 0, IPC_RMID) < 0) {
        perror("minishell: Failed to remove semaphore");
    }
}

void sem_wait_op(int semid) {
    struct sembuf sop;
    sop.sem_num = 0;
    sop.sem_op = -1; // P operation (decrement / wait)
    sop.sem_flg = 0;

    while (semop(semid, &sop, 1) == -1) {
        if (errno != EINTR) {
            perror("minishell: semop (wait) failed");
            exit(EXIT_FAILURE);
        }
    }
}

void sem_signal_op(int semid) {
    struct sembuf sop;
    sop.sem_num = 0;
    sop.sem_op = 1; // V operation (increment / signal)
    sop.sem_flg = 0;

    while (semop(semid, &sop, 1) == -1) {
        if (errno != EINTR) {
            perror("minishell: semop (signal) failed");
            exit(EXIT_FAILURE);
        }
    }
}

void worker_process(shared_data_t *shm, int semid, int use_semaphore) {
    for (int i = 0; i < INCREMENTS_PER_WORKER; i++) {
        if (use_semaphore) {
            sem_wait_op(semid);
        }

        // Read-modify-write (simulating a race condition if unprotected)
        int temp = shm->counter;
        // Introduce a tiny delay to ensure race condition happens consistently without semaphores
        for(volatile int j=0; j<100; j++); 
        shm->counter = temp + 1;

        if (use_semaphore) {
            sem_signal_op(semid);
        }
    }
    _exit(0);
}

void run_workers(shared_data_t *shm, int semid, int use_semaphore) {
    pid_t pids[NUM_WORKERS];

    shm->counter = 0;
    shm->mode = use_semaphore;

    for (int i = 0; i < NUM_WORKERS; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("minishell: fork failed in sync demo");
            break;
        } else if (pids[i] == 0) {
            // Child process
            worker_process(shm, semid, use_semaphore);
        }
    }

    // Wait for all workers to finish
    for (int i = 0; i < NUM_WORKERS; i++) {
        if (pids[i] > 0) {
            int status;
            waitpid(pids[i], &status, 0);
        }
    }
}

int cmd_race(void) {
    printf("Running Unsynchronized Race Condition Demo...\n");
    
    int shmid = create_shared_memory();
    if (shmid < 0) return -1;

    shared_data_t *shm = (shared_data_t *)shmat(shmid, NULL, 0);
    if (shm == (void *)-1) {
        perror("minishell: shmat failed");
        cleanup_shared_memory(shmid);
        return -1;
    }

    run_workers(shm, -1, 0); // No semaphore, use_semaphore = 0

    printf("Expected: %d, Got: %d\n", EXPECTED_TOTAL, shm->counter);
    if (shm->counter != EXPECTED_TOTAL) {
        printf("(RACE CONDITION! Data was lost due to concurrent access.)\n");
    } else {
        printf("(Incredibly lucky, no race condition observed this time. Try again!)\n");
    }

    shmdt(shm);
    cleanup_shared_memory(shmid);

    return 0;
}

int cmd_sync_demo(void) {
    printf("Running Synchronized Semaphore Demo...\n");
    
    int shmid = create_shared_memory();
    if (shmid < 0) return -1;

    int semid = create_semaphore();
    if (semid < 0) {
        cleanup_shared_memory(shmid);
        return -1;
    }

    shared_data_t *shm = (shared_data_t *)shmat(shmid, NULL, 0);
    if (shm == (void *)-1) {
        perror("minishell: shmat failed");
        cleanup_semaphore(semid);
        cleanup_shared_memory(shmid);
        return -1;
    }

    run_workers(shm, semid, 1); // With semaphore, use_semaphore = 1

    printf("Expected: %d, Got: %d\n", EXPECTED_TOTAL, shm->counter);
    if (shm->counter == EXPECTED_TOTAL) {
        printf("(CORRECT! Semaphore successfully protected the critical section.)\n");
    } else {
        printf("(INCORRECT! Something went wrong with the synchronization.)\n");
    }

    shmdt(shm);
    cleanup_semaphore(semid);
    cleanup_shared_memory(shmid);

    return 0;
}
