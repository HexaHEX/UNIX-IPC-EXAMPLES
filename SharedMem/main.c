#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>


#define    IS_READER_BUSY 0
#define    IS_WRITER_BUSY 1
#define    MUTEX 2
#define    DATA 3

int producer(const char*, int, void*);
int consumer(int, void*);
void err_sys(const char* error);


int main(int argc, const char* argv[]) {
    if((argc != 1) && (argc !=2)) {
        errno = EINVAL;
        err_sys("INCORRECT INPUT");
    }

    int semid = semget(0xAAA, 4, IPC_CREAT | 0666);
    if (semid < 0)
      err_sys("SEMGET ERROR");

    int shmid = shmget(0xAAA, 4096, IPC_CREAT | 0666);
    if(shmid < 0)
      err_sys("SHMGET ERROR");

    void* shm_ptr = shmat(shmid, NULL, 0);
    if(shm_ptr == (void*) -1)
      err_sys("SHMAT ERROR");

    switch(argc) {
        case 2:
            if (producer(argv[1], semid, shm_ptr) < 0)
                exit(EXIT_FAILURE);
            break;
        case 1:
            if (consumer(semid, shm_ptr) < 0)
                exit(EXIT_FAILURE);
        default:
            errno = EINVAL;
            exit(EXIT_FAILURE);
    }
}


#define SEM_ADD(_sem_num, _sem_op, _sem_flg)     do {\
                                                    sops[nsops].sem_num  = _sem_num; \
                                                    sops[nsops].sem_op   = _sem_op; \
                                                    sops[nsops].sem_flg  = _sem_flg; \
                                                    nsops++;\
                                                    } while(0)
// do semop with sops cmds and pop all of them
#define SEMOP()                                ({\
                                                    int _status = 0;\
                                                    assert(nsops < 16);\
                                                    _status = semop(semid, sops, nsops);\
                                                    nsops = 0;\
                                                    _status;})



int producer(const char* data, int semid, void* shm_ptr) {
    int data_fd = open(data, O_RDONLY);
    if(data_fd < 0)
      err_sys("OPEN ERROR");

    struct sembuf sops[16];
    int nsops = 0;

    SEM_ADD(IS_WRITER_BUSY, 0, 0);
    SEM_ADD(IS_READER_BUSY, 0, 0);
    SEM_ADD(IS_WRITER_BUSY, 2, SEM_UNDO);
    SEM_ADD(IS_WRITER_BUSY, -1, 0);
    SEMOP();
    // НАЧАЛО КРИТИЧЕСКОЙ СЕКЦИИ. ПИСАТЕЛИ БЬЮТСЯ ЗА ЧИТАТЕЛЯ

    SEM_ADD(IS_READER_BUSY, -1, 0);
    SEM_ADD(IS_READER_BUSY, 0, 0);
    SEM_ADD(IS_READER_BUSY,1, 0);
    SEM_ADD(DATA, 1, SEM_UNDO);
    SEM_ADD(DATA, -1, 0);
    SEMOP();

    int32_t bytes_read = 0;
    do {
        SEM_ADD(IS_READER_BUSY, -1, IPC_NOWAIT);
        SEM_ADD(IS_READER_BUSY, 0, IPC_NOWAIT);
        SEM_ADD(IS_READER_BUSY, 1, 0);
        SEM_ADD(DATA, 0, 0);
        SEM_ADD(MUTEX, 0, 0);
        SEM_ADD(MUTEX, 1, SEM_UNDO);
        if(SEMOP() < 0)
          err_sys("CONSUMER DIED");
      // НАЧАЛО КРИТИЧЕСКОЙ СЕКЦИИ. ПИСАТЕЛИ И ЧИТАТЕЛИ БЬЮТСЯ ЗА SHMEM

        bytes_read = read(data_fd, shm_ptr + sizeof(int32_t), 4096 - sizeof(int32_t));
        if(bytes_read < 0)
          err_sys("READ ERROR");

        *((int32_t*) shm_ptr) = bytes_read;


        SEM_ADD(DATA, 1, 0);
        SEM_ADD(MUTEX, -1, SEM_UNDO);
        SEM_ADD(MUTEX, 0, IPC_NOWAIT);
        if(SEMOP() < 0)
          err_sys("MUTEX WAS CORRUPTED");
      // КОНЕЦ КРИТИЧЕСКОЙ СЕКЦИИ. ПИСАТЕЛИ И ЧИТАТЕЛИ БЬЮТСЯ ЗА SHMEM

    } while(bytes_read != 0);

    SEM_ADD(IS_READER_BUSY, 0, 0);
    SEM_ADD(IS_WRITER_BUSY, -2, SEM_UNDO);
    SEM_ADD(DATA, 1, 0);
    SEM_ADD(DATA, -1, SEM_UNDO); //
    SEMOP();
    // КОНЕЦ КРИТИЧЕСКОЙ СЕКЦИИ. ПИСАТЕЛИ БЬЮТСЯ ЗА ЧИТАТЕЛЯ


    return 0;
}

int consumer(int semid, void* shm_ptr) {
    struct sembuf sops[16];
    int nsops = 0;

    SEM_ADD(IS_WRITER_BUSY, -1, 0);
    SEM_ADD(IS_WRITER_BUSY, 0, 0);
    SEM_ADD(IS_WRITER_BUSY, 2, 0);
    SEM_ADD(IS_READER_BUSY, 0 , 0);
    SEM_ADD(IS_READER_BUSY, 1, SEM_UNDO);
    SEMOP();
    // НАЧАЛО КРИТИЧЕСКОЙ СЕКЦИИ. ЧИТАТЕЛИ БЬЮТСЯ ЗА ПИСАТЕЛЯ

    int32_t read = 0;
    do {
        SEM_ADD(IS_WRITER_BUSY, -2, IPC_NOWAIT);
        SEM_ADD(IS_WRITER_BUSY, 0, IPC_NOWAIT);
        SEM_ADD(IS_WRITER_BUSY, 2, 0);
        SEM_ADD(DATA, -1, 0);
        SEM_ADD(DATA, 0, IPC_NOWAIT);
        SEM_ADD(MUTEX, 0, 0);
        SEM_ADD(MUTEX, 1, SEM_UNDO);
        if(SEMOP() < 0)
          err_sys("WRITER DIED");
        // НАЧАЛО КРИТИЧЕСКОЙ СЕКЦИИ. ЧИТАТЕЛИ И ПИСАТЕЛИ БЬЮТСЯ ЗА SHMEM

        read = *((int32_t*) shm_ptr);
        if(read < 0)
          err_sys("READ ERROR");

        if(write(STDOUT_FILENO, shm_ptr + sizeof(int32_t), read) < 0)
          err_sys("WRITE ERROR");

        SEM_ADD(MUTEX, -1, SEM_UNDO);
        SEM_ADD(MUTEX, 0, IPC_NOWAIT);
        // КОНЕЦ КРИТИЧЕСКОЙ СЕКЦИИ. ЧИТАТЕЛИ И ПИСАТЕЛИ БЬЮТСЯ ЗА SHMEM

        if(SEMOP() < 0)
          err_sys("MUTEX SEM WAS CORRUPTED");
    } while(read > 0);

    SEM_ADD(IS_READER_BUSY, -1, SEM_UNDO);
    SEMOP();
    // КОНЕЦ КРИТИЧЕСКОЙ СЕКЦИИ. ЧИТАТЕЛИ БЬЮТСЯ ЗА ПИСАТЕЛЯ


    return 0;
}

void err_sys(const char* error)
{
	perror(error);
	exit(1);
}
