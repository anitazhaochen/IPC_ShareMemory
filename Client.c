#include<stdio.h>
#include<stdlib.h>
#include<sys/shm.h>  // shared memory
#include<sys/sem.h>  // semaphore
#include<string.h>   // memcpy


// 联合体，用于semctl初始化
union semun
{
    int              val; /*for SETVAL*/
    struct semid_ds *buf;
    unsigned short  *array;
};

// P操作:
//  若信号量值为1，获取资源并将信号量值-1 
//  若信号量值为0，进程挂起等待
int sem_p(int sem_id)
{
    struct sembuf sbuf;
    sbuf.sem_num = 0; /*序号*/
    sbuf.sem_op = -1; /*P操作*/
    sbuf.sem_flg = SEM_UNDO;

    if(semop(sem_id, &sbuf, 1) == -1)
    {
        perror("P operation Error");
        return -1;
    }
    return 0;
}

// V操作：
//  释放资源并将信号量值+1
//  如果有进程正在挂起等待，则唤醒它们
int sem_v(int sem_id)
{
    struct sembuf sbuf;
    sbuf.sem_num = 0; /*序号*/
    sbuf.sem_op = 1;  /*V操作*/
    sbuf.sem_flg = SEM_UNDO;

    if(semop(sem_id, &sbuf, 1) == -1)
    {
        perror("V operation Error");
        return -1;
    }
    return 0;
}


int main()
{
	key_t key;
	int shmid, semid, msqid;
	char *shm;
	int flag = 1; /*while循环条件*/

	// 获取key值
	if((key = ftok(".", 'z')) < 0)
	{
		perror("ftok error");
		exit(1);
	}

	// 获取共享内存
	if((shmid = shmget(key, 1024, 0)) == -1)
	{
		perror("shmget error");
		exit(1);
	}

	// 连接共享内存
	shm = (char*)shmat(shmid, 0, 0);
	if((int)shm == -1)
	{
		perror("Attach Shared Memory Error");
		exit(1);
	}

	// 获取信号量
	if((semid = semget(key, 0, 0)) == -1)
	{
		perror("semget error");
		exit(1);
	}
	
	// 写数据
	printf("Client starts after server: \n");
				sem_p(semid);  /*访问资源*/
        printf("Message: %s \n", shm);
				sem_v(semid);  /*释放资源*/
	printf("0. Quit\n");
	printf("1. Change message\n");
	printf("2. Check message\n");
	
	while(flag)
	{
		char c;
		printf("Enter your choice: ");
		scanf("%c", &c);
		switch(c)
		{
			case '0':
				flag = 0;
				break;
			case '1':
				printf("Enter message: ");
				sem_p(semid);  /*访问资源*/
				scanf("%s", shm);
				sem_v(semid);  /*释放资源*/
				/*清空标准输入缓冲区*/
				while((c=getchar())!='\n' && c!=EOF);
				break;
			case '2':
				sem_p(semid);  /*访问资源*/
        printf("Message: %s\n", shm);
				sem_v(semid);  /*释放资源*/
				/*清空标准输入缓冲区*/
				while((c=getchar())!='\n' && c!=EOF);
        break;
			default:
				printf("Wrong input!\n");
				/*清空标准输入缓冲区*/
				while((c=getchar())!='\n' && c!=EOF);
		}
	}

	// 断开连接
	shmdt(shm);

	return 0;
}
