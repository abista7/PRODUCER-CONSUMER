#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <math.h>
#include <sys/time.h>

int *buffer;
int producerNum;
int consumerNum;
int itemConsumed;
int amountBuff;
int timeProduced;
int timeConsumed;
int itemsCount = 1;
int buffCapacity = 0;
int countPro = 1;
int countCons = 1;
int itemsInBuffer = 1;
int buffSize = 0;
int in = 0;
int out = 0;
int *arrayOfProducer;
int *arrayOfConsumer;
int buffIndexConsumer = 0;
int buffIndexProducer = 0;

sem_t full;
sem_t empty;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; 

typedef struct consInIt
{
    int pFillCount;
    int cRemCount;
    int cID;
    int cTime; 
} consumers;

typedef struct prodInIt
{
    int pFillCount;
    int pRemCount;
    int pId;
    int pTime;

} producers;


void *grab_item(void *param)
{
    consumers *grab_item = (consumers *)param;
    int size = grab_item->pFillCount;
    int cmsID = grab_item->cID;
    int sum = 0;

    while (sum++ < size)
    {
        sleep(grab_item->cTime);
        sem_wait(&full);
        pthread_mutex_lock(&m);
        printf("\n%d was consumed by consumer-> %d", buffer[buffIndexConsumer], cmsID + 1);

        arrayOfConsumer[countCons] = buffer[buffIndexConsumer];
        countCons++;

        if (buffIndexConsumer == (amountBuff - 1))
        {
            buffIndexConsumer = 0;
        }
        else
        {
            buffIndexConsumer++;
        }

        pthread_mutex_unlock(&m);
        sem_post(&empty);
    }

    pthread_exit(0);
}

void *put_item(void *param)
{
    producers *put_item = (producers *)param;
    int size = put_item->pFillCount;
    int prodId = put_item->pId;

    int sum = 0;

    while (sum++ < size)
    {
        sem_wait(&empty);
        pthread_mutex_lock(&m);
        printf("\n%d was produced by producer-> %d", itemsCount, prodId + 1);
        arrayOfProducer[countPro] = itemsCount;
        buffer[buffIndexProducer] = itemsCount;
        itemsCount++;
        countPro++;
        buffCapacity++;

        if (buffIndexProducer == (amountBuff - 1))
        {
            buffIndexProducer = 0;
        }
        else
        {
            buffIndexProducer++;
        }

        sleep(put_item->pTime);

        pthread_mutex_unlock(&m);
        sem_post(&full);
    }
    pthread_exit(0);
}

int main(int argc, char const *argv[])
{
    time_t start, end;               
    struct timeval tvBegin, tvStop; 
    int startTime, endTime;
    int errThreadCode;
    int pointer;

    producers *producerInfo;
    consumers *consumerInfo;

    pthread_t *pThread;
    pthread_t *cThread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_mutex_init(&m, NULL);


    if (argc != 7)
    {
        fprintf(stderr, "Invalid Input: please follow the following pattern:" 
                        "\n./pandc amountBuff,  producerNum consumnerNum itemsInBuffer timeProduced timeConsumed\n");    
        return EXIT_FAILURE;
    }


    start = time(NULL); 
    printf("\nCurrent Time: %s\n", ctime(&start));

    amountBuff = atoi(argv[1]);
    printf("                                  Numbers of Buffers : %d ", amountBuff); 
    producerNum = atoi(argv[2]);
    printf("\n                                Numbers of Producers : %d ", producerNum);
    consumerNum = atoi(argv[3]);
    printf("\n                                Numbers of Consumers : %d ", consumerNum);
    itemsInBuffer = atoi(argv[4]);
    printf("\n   Number of items each Producer Thread will Produce : %d ", itemsInBuffer);
    itemConsumed = (producerNum * itemsInBuffer / consumerNum);

    timeProduced = atoi(argv[5]);
    printf("\n                           Time each Producers sleep : %d ", timeProduced);
    timeConsumed = atoi(argv[6]);
    printf("\n                           Time each Consumers sleep : %d\n", timeConsumed);


    buffer = calloc(amountBuff, sizeof(int));
    arrayOfProducer = (int *)malloc(sizeof(int) * (producerNum * itemsInBuffer));
    arrayOfConsumer = (int *)malloc(sizeof(int) * (consumerNum * itemConsumed));

    pThread = (pthread_t *)malloc(producerNum * sizeof(pthread_t));

    if (pThread == NULL)
    {
        fprintf(stderr, "ERROR: memory allocation failed for pThread");
        return EXIT_FAILURE;
    }

    cThread = (pthread_t *)malloc(consumerNum * sizeof(pthread_t));
    if (cThread == NULL)
    {
        fprintf(stderr, "ERROR: memory allocation failed for cThread");
        return EXIT_FAILURE;
    }

    producerInfo = (producers *)malloc(producerNum * sizeof(producers));
    if (producerInfo == NULL)
    {
        fprintf(stderr, "ERROR: memory allocation failed for producerInfo");
        return EXIT_FAILURE;
    }

    consumerInfo = (consumers *)malloc(consumerNum * sizeof(consumers));
    if (consumerInfo == NULL)
    {
        fprintf(stderr, "ERROR: memory allocation failed for consumerInfo");
        return EXIT_FAILURE;
    }

    sem_init(&full, 0, 0);
    sem_init(&empty, 0, amountBuff);


    errThreadCode = pthread_mutex_init(&m, NULL);
    if (errThreadCode)
    {
        fprintf(stderr, "ERROR: return code is: %d\n", errThreadCode);
        return EXIT_FAILURE;
    }

    

    gettimeofday(&tvBegin, NULL);
    startTime = tvBegin.tv_sec;

    pointer = 0;
    while (pointer < producerNum)
    {
        producerInfo[pointer].pId = pointer;
        producerInfo[pointer].pRemCount = amountBuff;
        producerInfo[pointer].pFillCount = itemsInBuffer;
        producerInfo[pointer].pTime = timeProduced;

        errThreadCode = pthread_create(&pThread[pointer], &attr, put_item, &producerInfo[pointer]);
        if (errThreadCode != 0)
        {
            fprintf(stderr, "ERROR: producer return code is: %d\n", errThreadCode);
            return EXIT_FAILURE;
        }

        pointer++;
    }

    pointer = 0;
    while (pointer < consumerNum)
    {
        consumerInfo[pointer].cID = pointer;
        consumerInfo[pointer].cRemCount = amountBuff;
        consumerInfo[pointer].pFillCount = itemConsumed;
        consumerInfo[pointer].cTime = timeConsumed;

        errThreadCode = pthread_create(&cThread[pointer], &attr, grab_item, &consumerInfo[pointer]);
        if (errThreadCode != 0)
        {
            fprintf(stderr, "ERROR: consumer return code is: %d\n", errThreadCode);
            return EXIT_FAILURE;
        }

        pointer++;
    }

    for (int i = 0; i < consumerNum; i++)
    {
        errThreadCode = pthread_join(cThread[i], NULL);
        printf("\n     Consumer Thread Joined:  %d", (i + 1));

        if (errThreadCode != 0)
        {
            fprintf(stderr, "ERROR: producer return code is: %d\n", errThreadCode);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < producerNum; i++)
    {
        errThreadCode = pthread_join(pThread[i], NULL);
        printf("\n     Producer Thread Joined:  %d", (i + 1));

        if (errThreadCode != 0)
        {
            fprintf(stderr, "ERROR: producer return code is: %d\n", errThreadCode);
            return EXIT_FAILURE;
        }
    }

    pointer = 0;
    while (pointer < consumerNum)
    {
        if (errThreadCode != 0)
        {
            fprintf(stderr, "ERROR: consumer return code is: %d\n", errThreadCode);
            return EXIT_FAILURE;
        }

        pointer++;
    }

    pointer = 0;
    while (pointer < producerNum)
    {

        if (errThreadCode != 0)
        {
            fprintf(stderr, "ERROR: producer return code is: %d\n", errThreadCode);
            return EXIT_FAILURE;
        }

        pointer++;
    }

    
    end = time(NULL);
    printf("\nEnd time: %s\n", ctime(&end));
    gettimeofday(&tvStop, NULL);
    fprintf(stderr, "Producer Array  | Consumer Array\n");
    for (int i = 0; i < producerNum * itemsInBuffer; i++)
    {
        fprintf(stderr, "%-15d | %-15d\n", arrayOfProducer[i + 1], arrayOfConsumer[i + 1]);
    }

    if (countPro == countCons)
    {
        fprintf(stderr, "\nProducer and Consumer Arrays Match!\n");
    }
    else
    {
        fprintf(stderr, "\nProducer and Consumer Arrays Didn't MAtch!\n");
    }

    endTime = tvStop.tv_sec;
    printf("Total time taken: %i secs\n", (endTime - startTime));

    sem_destroy(&full);
    sem_destroy(&empty);
    free(pThread);
    free(producerInfo);
    free(cThread);
    free(consumerInfo);
    free(buffer);
    pthread_mutex_destroy(&m);

    return EXIT_SUCCESS;
}