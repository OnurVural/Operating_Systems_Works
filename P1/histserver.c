#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <time.h>
#include "histparam.h"
#include "histchild.h"
#include "histresult.h"

// This function reads the contents of the given file line by line.
void computeHistogramFromFile(char* inputFileName, int iCount, int iWidth, int iStart, mqd_t messageQueue, struct childMsg childMsg) {

    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    int intervals[iCount][3];
    int offset = 0;

    for (int i = 0; i < iCount; i++) {
        intervals[i][0] = iStart + offset;
        intervals[i][1] = (iStart + iWidth) + offset;
        intervals[i][2] = 0;
        offset = iWidth + offset;
    }

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

    int n;

    for (int i = 0; i < iCount; i++) {
        childMsg.noOfIntegers = intervals[i][2];
        childMsg.intervalStart = intervals[i][0];
        childMsg.intervalEnd = intervals[i][1];
        childMsg.index = i;

        n = mq_send(messageQueue, (char *) &childMsg, sizeof(struct childMsg), 0);

        if (n == -1) {
            perror("mq_send parent-child failed\n");
            exit(1);
        }

        //printf("mq_send success\n");
    }

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
        // Create message queue for child and parent
        mqd_t messageQueue;
        struct childMsg childMsg;

        messageQueue = mq_open(MQNAME1, O_RDWR | O_CREAT, 0666, NULL);
        if (messageQueue == -1) {
            perror("cannot create msg queue parent-child\n");
            exit(1);
        }
        printf("mq created parent-child, mq id = %d\n", (int) messageQueue);

        // Create children
        // const int noOfFiles = atoi(argv[1]);

        int histogram[iCount][3];
        int offset = 0;

        for (int i = 0; i < iCount; i++) {
            histogram[i][0] = iStart + offset;
            histogram[i][1] = (iStart + iWidth) + offset;
            histogram[i][2] = 0;
            offset = iWidth + offset;
        }

        //clock_t start, end;
        //double cpu_time_used;
     
        //start = clock();

        for (int i = 2; i < noOfFiles + 2; i++) {

            pid_t childProcessId = fork();

            if (childProcessId < 0) {
                printf("\nerror creating child!");
                exit(1);
            } 
            else if (childProcessId == 0) { // Child Process
                computeHistogramFromFile(argv[i], iCount, iWidth, iStart, messageQueue, childMsg);
                _exit(0);
            } 
            else { // Parent Process
                // Receive data from children and merge them
                struct mq_attr mq_attrC;
                struct childMsg *childMsgPtr;
                int x;
                char *bufptrC;
                int buflenC;
                mq_getattr(messageQueue, &mq_attrC);
                buflenC = mq_attrC.mq_msgsize;
                bufptrC = (char *) malloc(buflenC);

                for (int k = 0; k < iCount; k++) {
                    x = mq_receive(messageQueue, (char *) bufptrC, buflenC, NULL);
                    if (x == -1) {
                        perror("mq_receive parent-child failed\n");
                        exit(1);
                    }

                    //printf("mq_receive success, message size = %d\n", x);

                    childMsgPtr = (struct childMsg *) bufptrC;

                    //printf("received childMsgPtr->noOfIntegers = %d\n", childMsgPtr->noOfIntegers);
                    //printf("received childMsgPtr->intervalStart = %d\n", childMsgPtr->intervalStart);
                    //printf("received childMsgPtr->intervalEnd = %d\n", childMsgPtr->intervalEnd);
                    //printf("\n");

                    // Merging the results
                    histogram[childMsgPtr->index][2] = histogram[childMsgPtr->index][2] + childMsgPtr->noOfIntegers;
                }
                free(bufptrC);
            }
            //wait(0);
        }

        //end = clock();
        //cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        //printf("Time took: %f\n", cpu_time_used);

        mq_close(messageQueue);

        // Create the second message queue between server and client for sending the histogram data
        mqd_t mqH;
        struct histMsg histMsg;

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

    // Wait for all the children to terminate and then terminate server itself
    for (int i = 0; i < noOfFiles; ++i) {
               wait(NULL); 
    }

	return 0;
}