#ifndef SYNC_DEMO_H
#define SYNC_DEMO_H

typedef struct {
    int counter;
    int mode; // 0 = unsynchronized, 1 = synchronized
} shared_data_t;

// Expose these for builtins.c
int cmd_race(void);
int cmd_sync_demo(void);

// Internal helpers
int create_shared_memory(void);
void cleanup_shared_memory(int shmid);
int create_semaphore(void);
void cleanup_semaphore(int semid);
void sem_wait_op(int semid);
void sem_signal_op(int semid);
void run_workers(shared_data_t *shm, int semid, int use_semaphore);
void worker_process(shared_data_t *shm, int semid, int use_semaphore);

#endif // SYNC_DEMO_H
