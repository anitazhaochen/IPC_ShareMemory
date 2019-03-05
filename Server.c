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

// 初始化信号量
int init_sem(int sem_id, int value)
{
    union semun tmp;
    tmp.val = value;
    if(semctl(sem_id, 0, SETVAL, tmp) == -1)
    {
        perror("Init Semaphore Error");
        return -1;
    }
    return 0;
}

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

// 删除信号量集
int del_sem(int sem_id)
{
    union semun tmp;
    if(semctl(sem_id, 0, IPC_RMID, tmp) == -1)
    {
        perror("Delete Semaphore Error");
        return -1;
    }
    return 0;
}

// 创建一个信号量集
int creat_sem(key_t key)
{
	int sem_id;
	if((sem_id = semget(key, 1, IPC_CREAT|0002)) == -1)
	{
		perror("semget error");
		exit(-1);
	}
	init_sem(sem_id, 1);  /*初值设为1资源未占用*/
	return sem_id;
}


int main()
{
	key_t key;
	int shmid, semid, msqid;
	char *shm;
	char data[] = "this is server";
	struct shmid_ds buf1;  /*用于删除共享内存*/

	// 获取key值
	if((key = ftok(".", 'z')) < 0)
	{
		perror("ftok error");
		exit(1);
	}

	// 创建共享内存
	if((shmid = shmget(key, 1024, IPC_CREAT|0666)) == -1)
	{
		perror("Create Shared Memory Error");
		exit(1);
	}

	// 连接共享内存
	shm = (char*)shmat(shmid, 0, 0);
	if((int)shm == -1)
	{
		perror("Attach Shared Memory Error");
		exit(1);
	}

	// 创建信号量
	semid = creat_sem(key);
	
  printf("Server starts:\n");

  char c;
  int flag = 1;
  //sem_p(semid);  /*访问资源*/
  //shm = "Message: Hello from the server!";
  //sem_v(semid);  /*释放资源*/
	while(flag)
	{
    printf("0. Quit\n");
    printf("1. Change message\n");
    printf("2. Check message\n\n\n");
    printf("Enter your choice:");
    scanf("%c", &c);
    switch(c)
    {
      case '0':
        flag = 0;
        break;
      case '1':
         printf("Enter message: ");
         sem_p(semid);  /*访问资源*/
	       shmctl(shmid, IPC_SET, &buf1);
         scanf("%s", shm);
         sem_v(semid);  /*释放资源*/
         /*清空标准输入缓冲区*/
         while((c=getchar())!='\n' && c!=EOF);
        break;
      case '2':
			  sem_p(semid);
			  printf("Message: %s\n",shm);
			  sem_v(semid);
        while((c=getchar())!='\n' && c!=EOF);
        break;
      default:
        printf("Wrong input! \n");
        while((c=getchar())!='\n' && c!=EOF);
    }
	}

	// 断开连接
	shmdt(shm);

    /*删除共享内存、消息队列、信号量*/
	shmctl(shmid, IPC_RMID, &buf1);
	del_sem(semid);
	return 0;
}
