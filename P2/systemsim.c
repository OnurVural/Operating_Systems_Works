#include <stdbool.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define BUFSIZE 100
#define ln(x) log(x)

enum State {READY, RUNNING, WAITING};

int uniform_distribution(int rangeLow, int rangeHigh) {

    double myRand = rand() / (1.0 + RAND_MAX); 
    int range = rangeHigh - rangeLow + 1;
    int myRand_scaled = (myRand * range) + rangeLow;
    return myRand_scaled;
}

struct PCB {
    unsigned int pid;   
    pthread_t tid;
    int nextCPUBurstLength;
    int remainingCPUBurstLength;
    int noOfBurstsSoFar;
    int timeSpentInReadyList;
    int noOfTimesDevice1;
    int noOfTimesDevice2;
    int startArrivalTime;
    int finishTime;
    int totalExecTime;
    enum State state;
    int waitTime;
    int turnaroundTime;

    struct PCB* next;
    //struct PCB* prev;
};

struct Queue {
    struct PCB* head;
    struct PCB* tail;
    int size;
};

struct ReadyQueue {
    pthread_cond_t tcond_rqHasItem;
    pthread_cond_t tcond_rqHasSpace;
    pthread_mutex_t mutex_rq;
    struct Queue* queue;
};

//----------------------------------------------------------------------------------------------------------------

struct ReadyQueue *readyQ;


static int myPid = 0;
char* burst_dist;
int burstlen; 
int min_burst;
int max_burst;
int selection;

char *ALG;
char *quantum;
int MAXP;
int ALLP;
double pg;

double p0;
double p1;
double p2;

int t1;
int t2;

int OUTMODE;

int correctProcessCheck;

struct CPUSch {
    pthread_cond_t  CPUSch_cond;
    pthread_mutex_t CPUSch_mutex;
};

struct IO1 {
    pthread_cond_t  IO1_cond;
    pthread_mutex_t IO1_mutex;
    int size1;
};

struct IO2 {
    pthread_cond_t  IO2_cond;
    pthread_mutex_t IO2_mutex;
    int size2;
};

struct printRecord {
    int pid;
    int arv;
    int dept;
    int cpu;
    int waitr;
    int turna;
    int n_bursts;
    int n_d1;
    int n_d2; 
    int size;
    struct printRecord* next;
};

struct printQ {
    struct printRecord* head;
    struct printRecord* tail;
    int size;
};

struct printQ* prQ;
struct printRecord* prR;

void queue_init_pr(struct printQ *q)
{
        q->size = 0;
        q->head = NULL;
        q->tail = NULL;
}

// this function assumes that space for item is already allocated
void queue_insert_pr(struct printQ *q, struct printRecord *qe)
{
    if (q->size == 0) {
            q->head = qe;
            q->tail = qe;
    } else {
            q->tail->next = qe;
            q->tail = qe;
    }

    q->size++;
}

// this function does not free the item
// it is deleting the item from the list 
struct printRecord *queue_retrieve_pr(struct printQ *q)
{
    struct printRecord *qe;

    if (q->size == 0)
            return NULL;

    qe = q->head;
    return (qe);
}

struct CPUSch* cpu;
struct IO1* io1;
struct IO2* io2;


void queue_init(struct Queue *q)
{
        q->size = 0;
        q->head = NULL;
        q->tail = NULL;
}

// this function assumes that space for item is already allocated
void queue_insert(struct Queue *q, struct PCB *qe)
{
    if (q->size == 0) {
            q->head = qe;
            q->tail = qe;
    } else {
            q->tail->next = qe;
            q->tail = qe;
    }

    q->size++;
}

// this function does not free the item
// it is deleting the item from the list 
struct PCB *queue_retrieve(struct Queue *q)
{
    struct PCB *qe;

    if (q->size == 0)
            return NULL;

    qe = q->head;
    return (qe);
}

void enqueue(struct ReadyQueue* readyQ, struct PCB* pcb)
{
	pthread_mutex_lock(&readyQ->mutex_rq); // ERROR

    /* critical section begin */
	while (readyQ->queue->size == BUFSIZE) {
        pthread_cond_wait(&readyQ->tcond_rqHasSpace, &readyQ->mutex_rq);
    }

	queue_insert(readyQ->queue, pcb);
		
	if (readyQ->queue->size == 1) {
        pthread_cond_signal(&readyQ->tcond_rqHasItem);
    }
	
	/* critical section end */
	pthread_mutex_unlock(&readyQ->mutex_rq);
}

struct PCB *dequeue(struct ReadyQueue *readyQ)
{
	struct PCB *pcb;
	
	pthread_mutex_lock(&readyQ->mutex_rq);

	/* critical section begin */
	while (readyQ->queue->size == 0) {
		pthread_cond_wait(&readyQ->tcond_rqHasItem, &readyQ->mutex_rq);
	}

	pcb = queue_retrieve(readyQ->queue);
	
	if (pcb == NULL) {
		printf("can not retrieve; should not happen\n");
		exit(1);
	}
	
	if (readyQ->queue->size == (BUFSIZE - 1)) {
        pthread_cond_signal(&readyQ->tcond_rqHasSpace);
    }
	
	/* critical section end */
	pthread_mutex_unlock(&readyQ->mutex_rq);
	return (pcb); 
}

/*struct PCB* newNode(pthread_t tid) {
    
    struct PCB* temp = (struct PCB*)malloc(sizeof(struct PCB));
    temp->tid = tid;
    temp->next = NULL;

    temp->pid = myPid; 
    myPid++;

    temp->state = READY;
    
    if (strcmp("fixed", burst_dist) == 0) {
        temp->nextCPUBurstLength = burstlen;
        temp->remainingCPUBurstLength = burstlen;
    }

    if (strcmp("exponential", burst_dist) == 0) {

        double lambda;
        double random;
        int x;
        
        do {
            lambda = 1 / burstlen;
            random =  rand() / ((double) RAND_MAX);
            x = (int) ((-1) * ln(random)) / (lambda);    
        } while ( min_burst > x || x > max_burst );
        
        temp->nextCPUBurstLength = x;
        temp->remainingCPUBurstLength = x;
    }

    if (strcmp("uniform", burst_dist) == 0) {
        int x;
        x = uniform_distribution( min_burst, max_burst );
        temp->nextCPUBurstLength = x;
        temp->remainingCPUBurstLength = x;
    }
    
    temp->noOfBurstsSoFar = 0;
    temp->timeSpentInReadyList = 0;
    temp->noOfTimesDevice1 = 0;
    temp->noOfTimesDevice2 = 0;

    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    temp->startArrivalTime = current_time.tv_usec;
    temp->finishTime = -1;
    temp->totalExecTime = 0;
  
    return temp;
}*/

struct ReadyQueue* createQueue() {

    struct ReadyQueue* readyQ = (struct ReadyQueue*)malloc(sizeof(struct ReadyQueue));
    readyQ->queue->head = readyQ->queue->tail = NULL;
    return readyQ;
}

// Arguments of enqueue
/*struct arg {
    struct ReadyQueue* readyQ;
    struct PCB* pcb;
    pthread_t tid;
};*/

void queueSort(struct Queue* queue) {
    struct PCB **pp, *this;

    if (!queue->head ) {
        fprintf(stderr, "Struct is empty\n");
        return;
    }

    for(pp = &queue->head; this = *pp; pp = (&(*pp)->next)) {
        struct PCB *other = this->next;
        if (!this->next) break;
        if (this->remainingCPUBurstLength < other->remainingCPUBurstLength) continue;

        *pp = other;              
        this->next = other->next; 
        other->next = this;   
    }

    return;
}

// Arguments of processGeneratorThread
struct argProcessGenerator {

};

// Arguments of CPUSchedulerThread
struct argCPUScheduler {
    
};

// Arguments of processThread 
struct argProcessThread {
    struct ReadyQueue* readyQ;
    pthread_t tid;
};

void *processThread(void *argProcessThread) {

    struct PCB* temp = (struct PCB*)malloc(sizeof(struct PCB));

    struct timeval create;
    gettimeofday(&create, NULL);
    temp->startArrivalTime = create.tv_usec;
    
    temp->tid = ((struct argProcessThread *) argProcessThread)->tid;
    temp->next = NULL;

    temp->pid = myPid; 
    myPid++;

    if (strcmp("fixed", burst_dist) == 0) {

        temp->nextCPUBurstLength = burstlen;
        temp->remainingCPUBurstLength = burstlen;
    }

    if (strcmp("exponential", burst_dist) == 0) {

        double lambda;
        double random;
        int x;
        
        do {
            lambda = 1 / burstlen;
            random =  rand() / ((double) RAND_MAX);
            x = (int) ((-1) * ln(random)) / (lambda);    
        } while ( min_burst > x || x > max_burst );
        
        temp->nextCPUBurstLength = x;
        temp->remainingCPUBurstLength = x;
    }

    if (strcmp("uniform", burst_dist) == 0) {
        int x;
        x = uniform_distribution( min_burst, max_burst );
        temp->nextCPUBurstLength = x;
        temp->remainingCPUBurstLength = x;
    }

    temp->noOfBurstsSoFar = 0;
    temp->timeSpentInReadyList = 0;

    //struct timeval current_time;
    //gettimeofday(&current_time, NULL);
    
    temp->finishTime = -1;
    temp->totalExecTime = 0;

    pthread_mutex_lock(&readyQ->mutex_rq);

    temp->state = READY;
    temp->noOfTimesDevice1 = 0;
    temp->noOfTimesDevice2 = 0;
    enqueue(readyQ, temp);
    pthread_cond_wait(&readyQ->tcond_rqHasItem, &readyQ->mutex_rq);
    
    pthread_mutex_unlock(&readyQ->mutex_rq);
                        
    pthread_mutex_lock(&readyQ->mutex_rq);

    //double random =  rand() / ((double) RAND_MAX);
    int prob0 = 1 / p0;
    int prob1 = 1 / p1;
    int prob2 = 1 / p2;

    do {
        if (temp->pid == correctProcessCheck) {

            if (rand() % prob0 == 0) {
                temp->noOfBurstsSoFar++;
                break;
            }

            if (rand() % prob1 == 0) {

                pthread_mutex_lock(&io1->IO1_mutex);
                temp->noOfTimesDevice1++;

                while (io1->size1 > 0) {
                    pthread_cond_wait(&io1->IO1_cond, &io1->IO1_mutex);
                }

                (io1->size1)++;

                pthread_mutex_unlock(&io1->IO1_mutex);
                usleep(t1);
                pthread_mutex_lock(&io1->IO1_mutex);
                (io1->size1)--;
                pthread_mutex_unlock(&io1->IO1_mutex);
                pthread_cond_signal(&io1->IO1_cond);
                enqueue(readyQ, temp);
                pthread_cond_signal(&cpu->CPUSch_cond);
            }

            if (rand() % prob2 == 0) {

                pthread_mutex_lock(&io2->IO2_mutex);
                temp->noOfTimesDevice2++;

                while (io2->size2 > 0) {
                    pthread_cond_wait(&io2->IO2_cond, &io2->IO2_mutex);
                }

                (io2->size2)++;

                pthread_mutex_unlock(&io2->IO2_mutex);
                usleep(t2);
                pthread_mutex_lock(&io2->IO2_mutex);
                (io2->size2)--;
                pthread_mutex_unlock(&io2->IO2_mutex);
                pthread_cond_signal(&io2->IO2_cond);
                enqueue(readyQ, temp);
                pthread_cond_signal(&cpu->CPUSch_cond);
            }

        }
        pthread_cond_wait(&readyQ->tcond_rqHasItem, &readyQ->mutex_rq);

    } while(1);

    pthread_mutex_unlock(&readyQ->mutex_rq);

    struct timeval beginCPU;
    struct timeval endCPU; 
    struct timeval finish;

    gettimeofday(&beginCPU, NULL);

    if (strcmp(ALG, "RR") == 0) {
        usleep(atoi(quantum));
    }

    else {
        usleep(temp->remainingCPUBurstLength);
    }

    gettimeofday(&endCPU, NULL);

    double cpu_time = (endCPU.tv_sec - beginCPU.tv_sec) * 1000;
    temp->totalExecTime = cpu_time;
    temp->remainingCPUBurstLength = temp->nextCPUBurstLength - cpu_time;

    pthread_cond_signal(&cpu->CPUSch_cond);
    
    gettimeofday(&finish, NULL);
    double total_time = (finish.tv_sec - create.tv_sec) * 1000;
    temp->turnaroundTime = total_time;
    temp->waitTime = total_time - cpu_time;

    if (OUTMODE == 1) {
        //print();
    }

    if (OUTMODE == 2) {
        struct timeval current_time;
        gettimeofday(&current_time, NULL);
        double cur = (current_time.tv_sec) * 1000;
        printf("Current Time: %f pID: %d State: %u\n", cur, temp->pid, temp->state);
    }

    if (OUTMODE == 3) {
        printf("BURAYI ANLAMADIK HOCAM\n");
    }

    //struct printRecord* prR;
    prR->pid = temp->pid;
    prR->arv = temp->startArrivalTime;
    prR->dept = temp->finishTime;
    prR->cpu = temp->totalExecTime;
    prR->turna = temp->turnaroundTime;
    prR->n_bursts = temp->noOfBurstsSoFar;
    prR->n_d1 = temp->noOfTimesDevice1;
    prR->n_d2 = temp->noOfTimesDevice2;
    prR->waitr = temp->waitTime;

    queue_insert_pr(prQ, prR);

    pthread_exit(NULL);

    /*int startArrivalTime;
    int finishTime;
    int totalExecTime;
    int waitTime;
    int turnaroundTime;*/

    /** OUTMODE 2
    the running thread will print out the current time (in ms
    from the start of the simulation), its pid and its state to the screen
     */

    /*: OUTMODE 3
    new process
    created, process will be added to the ready queue, process is selected for CPU,
    process is running in CPU, process will be added to the device queue (i.e., the
    related condition variable’s internal queue), process doing I/O with the
    device, process expired time quantum, process finished, etc.*/
}

void *processGeneratorThread(void *argProcessGenerator) {

    pthread_t tids[ALLP]; // Thread IDs
    struct argProcessThread t_args[ALLP]; // Thread function arguments
    int ret;

    int prob = 1 / pg;

    if (MAXP < 10) {

        for (int i = 0; i < MAXP; i++) {
            ret = pthread_create(&(tids[i]), NULL, processThread, (void *) &(t_args[i]));
            
            if (ret != 0) {
                printf("thread create failed \n");
                exit(1);
            }
            printf("thread %i with tid %u created\n", i, (unsigned int) tids[i]);
        }

        for (int i = MAXP; i < ALLP; i++) {
            usleep(5000);
            
            if (i < MAXP && rand() % prob == 0) {
                //t_args[i].pcb = newNode(i);
                t_args[i].tid = i;
                t_args[i].readyQ = readyQ;

                ret = pthread_create(&(tids[i]), NULL, processThread, (void *) &(t_args[i]));
                
                if (ret != 0) {
                    printf("thread create failed \n");
                    exit(1);
                }
                printf("thread %i with tid %u created\n", i, (unsigned int) tids[i]);
            }
        }
    }

    if (MAXP >= 10) {

        for (int i = 0; i < 10; i++) {
            ret = pthread_create(&(tids[i]), NULL, processThread, (void *) &(t_args[i]));
            
            if (ret != 0) {
                printf("thread create failed \n");
                exit(1);
            }
            printf("thread %i with tid %u created\n", i, (unsigned int) tids[i]);
        }

        for (int i = 10; i < ALLP; i++) {
            usleep(5000);
            
            if (i < MAXP && rand() % prob == 0) {
                //t_args[i].pcb = newNode(i);
                t_args[i].tid = i;
                t_args[i].readyQ = readyQ;

                ret = pthread_create(&(tids[i]), NULL, processThread, (void *) &(t_args[i]));
                
                if (ret != 0) {
                    printf("thread create failed \n");
                    exit(1);
                }
                printf("thread %i with tid %u created\n", i, (unsigned int) tids[i]);
            }
        }
    }

    for (int i = 0; i < readyQ->queue->size; i++) {
        pthread_join(tids[i], NULL);
    }
}

void *CPUSchedulerThread(void *argCPUScheduler) { //char* ALG, int Q 

    while (1) {

        if (readyQ->queue->size != 0) {

            pthread_mutex_lock(&cpu->CPUSch_mutex);
            pthread_mutex_lock(&readyQ->mutex_rq);

            //while (readyQ->queue->size != 0) {
                pthread_cond_wait(&cpu->CPUSch_cond, &cpu->CPUSch_mutex);
            //}

            if (strcmp(ALG, "SJF") == 0) {
                // sort the queue
                queueSort(readyQ->queue);
            }

            // we assume queue has been sorted if ALG is SJF
            readyQ->queue->head->state = RUNNING;
            correctProcessCheck = readyQ->queue->head->pid;
            dequeue(readyQ);

            pthread_mutex_unlock(&readyQ->mutex_rq);
            pthread_cond_broadcast(&readyQ->tcond_rqHasItem);

            pthread_cond_wait(&cpu->CPUSch_cond, &cpu->CPUSch_mutex);
            pthread_mutex_unlock(&cpu->CPUSch_mutex);
        }
        else {
            break;
        }
    }

    pthread_exit(0);
}

int main(int argc, char **argv) {

    /*<ALG> <Q> <T1> <T2> <burst-dist> <burstlen> <min-burst>
    <max-burst> <p0> <p1> <p2> <pg> <MAXP> <ALLP> <OUTMODE>*/

    ALG = argv[1];
    quantum = argv[2]; // "INF", any integer
    t1 = atoi(argv[3]);
    t2 = atoi(argv[4]);
    burst_dist = argv[5];
    burstlen = atoi(argv[6]);
    min_burst = atoi(argv[7]);
    max_burst = atoi(argv[8]);
    p0 = atof(argv[9]);
    p1 = atof(argv[10]);
    p2 = atof(argv[11]);
    pg = atof(argv[12]);
    MAXP = atoi(argv[13]);
    ALLP = atoi(argv[14]);
    OUTMODE = atoi(argv[15]);

    // INITLEME SURECLERI ASAGIDADIR

    /* init Rqueueueueueueueueueueueueeuueeueu and mutex/condition variables */ 
    readyQ = (struct ReadyQueue*)malloc(sizeof(struct ReadyQueue));
    readyQ->queue= (struct Queue *) malloc(sizeof (struct Queue)); 
    queue_init(readyQ->queue);
    pthread_mutex_init(&readyQ->mutex_rq, NULL);
    pthread_cond_init(&readyQ->tcond_rqHasItem, NULL); 
    pthread_cond_init(&readyQ->tcond_rqHasSpace, NULL);

    // prQ initleme sureci
    prQ = (struct printQ *) malloc(sizeof (struct printQ)); 
    queue_init_pr(prQ);

    // cpu initleme sureci
    cpu = (struct CPUSch*)malloc(sizeof(struct CPUSch));
    pthread_mutex_init(&cpu->CPUSch_mutex, NULL);
    pthread_cond_init(&cpu->CPUSch_cond, NULL); 

    // io1 initleme sureci
    io1 = (struct IO1*)malloc(sizeof(struct IO1));
    pthread_mutex_init(&io1->IO1_mutex, NULL);
    pthread_cond_init(&io1->IO1_cond, NULL); 

    // io2 initleme sureci
    io2 = (struct IO2*)malloc(sizeof(struct IO2));
    pthread_mutex_init(&io2->IO2_mutex, NULL);
    pthread_cond_init(&io2->IO2_cond, NULL); 



    // METHODLARI CAGIRMA SURECI

    int pgt;
    int cpust;
    pthread_t threads[4];

    pgt = pthread_create(&threads[0], NULL, processGeneratorThread, NULL);

    if (pgt != 0) { 
        printf("can not create thread\n");
        exit(1);
    }

    cpust = pthread_create(&threads[1], NULL, CPUSchedulerThread, NULL);

    if (cpust != 0) { 
        printf("can not create thread\n");
        exit(1);
    }

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);

    struct printRecord* prRPointer;
    printf("pid   arv   dept   cpu   waitr   turna   n-bursts   n-d1   n-d2\n");

    printf("%d\n", prQ->size);

    for (int i = 0; i < prQ->size; i++) {
        prRPointer = queue_retrieve_pr(prQ);
        printf("%d    %d    %d    %d    %d    %d    %d    %d    %d\n", 
        prRPointer->pid, prRPointer->arv, prRPointer->dept, prRPointer->cpu, prRPointer->waitr, prRPointer->turna, prRPointer->n_bursts, prRPointer->n_d1, prRPointer->n_d2);
    }

    // DESTROYLAMA SURECLERI ASAGIDADIR

    /* destroy Rqueueueueueueueueueueueueeuueeueu and mutex/condition variables */ 
    pthread_mutex_destroy(&readyQ->mutex_rq); 
    pthread_cond_destroy(&readyQ->tcond_rqHasItem); 
    pthread_cond_destroy(&readyQ->tcond_rqHasSpace); 
    free(readyQ->queue);
    free(readyQ); 

    // prQ freeleme sureci
    free(prQ);

    // cpu destroylama sureci
    pthread_mutex_destroy(&cpu->CPUSch_mutex); 
    pthread_cond_destroy(&cpu->CPUSch_cond);
    free(cpu); 
 
    // io1 destroylama sureci
    pthread_mutex_destroy(&io1->IO1_mutex); 
    pthread_cond_destroy(&io1->IO1_cond);
    free(io1); 

    // io2 destroylama sureci
    pthread_mutex_destroy(&io2->IO2_mutex); 
    pthread_cond_destroy(&io2->IO2_cond);
    free(io2);
}