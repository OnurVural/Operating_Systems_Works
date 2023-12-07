#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "histparam.h"
#include "histchild.h"
#include "histresult.h"

#define MAXTHREADS  20		/* max number of threads */
#define MAXFILENAME 50		/* max length of a filename */

int arr[1000][2];

// arguments of computeHistogramFromFile
struct arg {
    char* inputFileName;
    int iCount;
    int iWidth;
    int iStart;
	int t_index;	/* the index of the created thread */
};

// This function reads the contents of the given file line by line.
static void *computeHistogramFromFile(void *arg_ptr) {

    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    printf("thread %d started\n", ((struct arg *) arg_ptr)->t_index); 

    int iCount = ((struct arg *) arg_ptr)->iCount;
    int iStart = ((struct arg *) arg_ptr)->iStart;
    int iWidth = ((struct arg *) arg_ptr)->iWidth;
    char* inputFileName = ((struct arg *) arg_ptr)->inputFileName;

    // create histogram buckets according to given arguments
    int intervals[iCount][3];
    int offset = 0;
    for (int i = 0; i < iCount; i++) {
        intervals[i][0] = iStart + offset;
        intervals[i][1] = (iStart + iWidth) + offset;
        intervals[i][2] = 0;
        offset = iWidth + offset;
    }

    // open file and place data to bucket
    fp = fopen(inputFileName, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        int number = atoi(line);
        for (int i = 0; i < iCount; i++) {
            if (intervals[i][0] <= number && intervals[i][1] > number)
                intervals[i][2]++;
        }
    } 

    for (int i = 0; i < iCount; i++) {
        arr[i][0] = arr[i][0] + intervals[i][2];
        arr[i][1] = i;
    }

    // business completed time to join
    char* retreason;
    retreason = malloc(200); 
	strcpy (retreason, "Work done: expected termination of thread..."); 
	pthread_exit(retreason);  // just tell a reason to the thread that is waiting in join

    fclose(fp);
    if (line)
        free(line);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    // Retrieve the message from the client side about the histogram parameters
	mqd_t mq;
	struct mq_attr mq_attr;
	struct msg *msgPtr;
	int n;
	char *bufptr;
	int buflen;

	mq = mq_open(MQNAME, O_RDWR | O_CREAT, 0666, NULL);
	if (mq == -1) {
		perror("cannot create msg queue server-client-1 \n");
		exit(1);
	}
	printf("mq created server-client-1, mq id = %d\n", (int) mq);

	mq_getattr(mq, &mq_attr);
	// allocate large enough space for the buffer to store an incoming message
    buflen = mq_attr.mq_msgsize;
	bufptr = (char *) malloc(buflen);

	n = mq_receive(mq, (char *) bufptr, buflen, NULL);
	if (n == -1) {
		perror("mq_receive server-client-1 failed\n");
		exit(1);
	}

	msgPtr = (struct msg *) bufptr;
    const int iCount = msgPtr->intervalCount;
    const int iStart = msgPtr->intervalStart;
    const int iWidth = msgPtr->intervalWidth;
    bool termination = msgPtr->termination;

	free(bufptr);
	mq_close(mq);

    if (termination) {
        printf("Error! Either one or both of the parameters to the client was incorrect. Exiting...\n");
        exit(0);
    }

    const int noOfFiles = atoi(argv[1]);

    do {
        
        int histogram[iCount][3];
        int offset = 0;
        for (int i = 0; i < iCount; i++) {
            histogram[i][0] = iStart + offset;
            histogram[i][1] = (iStart + iWidth) + offset;
            histogram[i][2] = 0;
            offset = iWidth + offset;
        }

        //-----------------------------------------------------------------------------------------------------------------------

        pthread_t tids[MAXTHREADS];	    //thread ids
        struct arg t_args[MAXTHREADS];	//thread function arguments
        
        int i;
        int ret;
        char *retmsg;

        //clock_t start, end;
        //double cpu_time_used;
     
        //start = clock();
    
        for (i = 0; i < noOfFiles; ++i) {
            // PREPARE ARGUMENTS
            t_args[i].inputFileName = argv[i+2];
            t_args[i].iCount = iCount;
            t_args[i].iWidth = iWidth;
            t_args[i].iStart = iStart;
            t_args[i].t_index = i;

            ret = pthread_create(&(tids[i]), NULL, computeHistogramFromFile, (void *) &(t_args[i]));
            
            if (ret != 0) {
                printf("thread create failed \n");
                exit(1);
            }
            printf("thread %i with tid %u created\n", i, (unsigned int) tids[i]);
        }
      
        printf("main: waiting all threads to terminate\n");
        for (i = 0; i < noOfFiles; ++i) {
            ret = pthread_join(tids[i], (void **)&retmsg);
            if (ret != 0) {
                printf("thread join failed \n");
                exit(1);
            }
            printf ("thread terminated, msg = %s\n", retmsg);
            // we got the reason as the string pointed by retmsg
            // space for that was allocated in thread function; now freeing. 
            free (retmsg); 
        }
        printf("main: all threads terminated\n");

        //end = clock();
        //cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        //printf("Time took: %f\n", cpu_time_used);

        //-----------------------------------------------------------------------------------------------------------------------

        // Create the second message queue between server and client for sending the histogram data
        mqd_t mqH;
        struct histMsg histMsg;

        for (int k = 0; k < iCount; k++) {
            histogram[k][2] =  arr[k][0];
            //printf("%d\n", histogram[k][2]);
        }

        int a;

        mqH = mq_open(MQNAME2, O_RDWR | O_CREAT, 0666, NULL);
        if (mqH == -1) {
            perror("cannot open msg queue server-client-2\n");
            exit(1);
        }
        printf("mq server-client-2 created, mq id = %d\n", (int) mqH);

        for (int i = 0; i < iCount; i++) {
            histMsg.intervalStart = histogram[i][0];
            histMsg.intervalEnd = histogram[i][1];
            histMsg.noOfIntegers = histogram[i][2];
            histMsg.bucketNo = i;

            a = mq_send(mqH, (char *) &histMsg, sizeof(struct histMsg), 0);

            if (a == -1) {
                perror("mq_send server-client-2 failed\n");
                exit(1);
            }
        }

        mq_close(mqH);

        // Recieve the termination message from the client
        mq = mq_open(MQNAME, O_RDWR);
        if (mq == -1) {
            perror("cannot create msg queue server-client-1 \n");
            exit(1);
        }
        printf("mq open server-client-1, mq id = %d\n", (int) mq);

        mq_getattr(mq, &mq_attr);
        //printf("mq maximum msgsize = %d\n", (int) mq_attr.mq_msgsize);

        // allocate large enough space for the buffer to store an incoming message
        buflen = mq_attr.mq_msgsize;
        bufptr = (char *) malloc(buflen);

        n = mq_receive(mq, (char *) bufptr, buflen, NULL);
        if (n == -1) {
            perror("mq_receive server-client-1 failed\n");
            exit(1);
        }

        //printf("mq_receive success, message size = %d\n", n);

        msgPtr = (struct msg *) bufptr;

        //printf("received msgPtr->intervalCount = %d\n", msgPtr->intervalCount);
        //printf("received msgPtr->intervalStart = %d\n", msgPtr->intervalStart);
        //printf("received msgPtr->intervalWidth = %d\n", msgPtr->intervalWidth);
        //printf("\n");

        termination = msgPtr->termination;

        free(bufptr);
        mq_close(mq);

        if (termination) {
            printf("TERMINATION\n");
        }

    } while(!termination);

	return 0;
}