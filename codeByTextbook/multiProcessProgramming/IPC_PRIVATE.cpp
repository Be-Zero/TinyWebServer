// IPC_PRIVATE.cpp
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
union semun
{
    int val;              // 信号量大小
    struct semid_ds *buf; // 信号量的数据结构，包含操作权限、数目、最后一次分别调用semop和semctl的时间
    unsigned short int *array;
    struct seminfo *__buf;
};
// op为-1时执行P操作，op为1时执行V操作
void pv(int sem_id, int op)
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = op;
    sem_b.sem_flg = SEM_UNDO;
    semop(sem_id, &sem_b, 1);
}
int main(int argc, char *argv[])
{
    int sem_id = semget(IPC_PRIVATE, 1, 0666); // 创建信号量集，1表示无论信号量是否存在，都将新建，且并非进程私有，2表示信号量集中的信号量个数
    union semun sem_un;
    sem_un.val = 1;
    semctl(sem_id, 0, SETVAL, sem_un); // 对信号量直接进行控制，1信号量集标识符，2信号量编号，3命令（设置值为sem_un）
    pid_t id = fork();                 // 创建进程
    if (id < 0)
    {
        return 1;
    }
    else if (id == 0) // 子进程
    {
        printf("child try to get binary sem\n");
        // 在父、子进程间共享IPC_PRIVATE信号量的关键就在于二者都可以操作该信号量的标识符sem_id
        pv(sem_id, -1); // P操作
        printf("child get the sem and would release it after 5 seconds\n");
        sleep(5);
        pv(sem_id, 1); // v操作
        exit(0);
    }
    else
    {
        printf("parent try to get binary sem\n");
        pv(sem_id, -1);
        printf("parent get the sem and would release it after 5 seconds\n");
        sleep(5);
        pv(sem_id, 1);
    }
    waitpid(id, NULL, 0);
    semctl(sem_id, 0, IPC_RMID, sem_un);
    // 删除信号量
    return 0;
}