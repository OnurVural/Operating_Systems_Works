#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "histparam.h"
#include "histresult.h"

int main(int argc, char **argv) {

    const int intervalCount = atoi(argv[1]);
    const int intervalWidth = atoi(argv[2]);
    const int intervalStart = atoi(argv[3]); 
    
    mqd_t mq;
    struct msg msg;
    msg.intervalCount = intervalCount;
    msg.intervalWidth = intervalWidth;
    msg.intervalStart = intervalStart;
    msg.termination = false;
    int n;

    mq = mq_open(MQNAME, O_RDWR);
    if (mq == -1) {
        perror("cannot open msg queue server-client-1\n");
        exit(1);
    }
    printf("mq server-client-1 opened, mq id = %d\n", (int) mq);

    if (intervalCount < 1 || intervalCount > 1000) {
        printf("Error! Interval Count not in range. Exiting...\n");
        msg.termination = true;
        n = mq_send(mq, (char *) &msg, sizeof(struct msg), 0);
        exit(0);
    }

    if (intervalWidth < 1 || intervalWidth > 1000000) {
        printf("Error! Interval Width not in range. Exiting...\n");
        msg.termination = true;
        n = mq_send(mq, (char *) &msg, sizeof(struct msg), 0);
        exit(0);
    } 

    n = mq_send(mq, (char *) &msg, sizeof(struct msg), 0);

    if (n == -1) {
        perror("mq_send failed\n");
        exit(1);
    }

    printf("mq_send server-client-1 success\n");
    sleep(1);
    mq_close(mq);

    // Recieve message from server about histogram data, print it, then send another message when done. after server terminates, terminate.
    mqd_t mqH;
	struct mq_attr mq_attrH;
	struct histMsg *histMsg;
	int x;
	char *bufptrH;
	int buflenH;

	mqH = mq_open(MQNAME2, O_RDWR);
	if (mqH == -1) {
		perror("cannot create msg queue server-client-2\n");
		exit(1);
	}
	printf("mq server-client-2 opened, mq id = %d\n", (int) mqH);

	mq_getattr(mqH, &mq_attrH);
	//printf("mq maximum msgsize = %d\n", (int) mq_attrH.mq_msgsize);

	// allocate large enough space for the buffer to store an incoming message
    buflenH = mq_attrH.mq_msgsize;
	bufptrH = (char *) malloc(buflenH);

    int printArr[intervalCount][3];
    int offset = 0;

    for (int i = 0; i < intervalCount; i++) {
        printArr[i][0] = intervalStart + offset;
        printArr[i][1] = (intervalStart + intervalWidth) + offset;
        printArr[i][2] = 0;
        offset = intervalWidth + offset;
    }

    for (int i = 0; i < intervalCount; i++) {
        x = mq_receive(mqH, (char *) bufptrH, buflenH, NULL);
        if (x == -1) {
            perror("mq_receive server-client-2 failed\n");
            exit(1);
        }

        histMsg = (struct histMsg *) bufptrH;

        printArr[histMsg->bucketNo][0] = histMsg->intervalStart;
        printArr[histMsg->bucketNo][1] = histMsg->intervalEnd;
        printArr[histMsg->bucketNo][2] = histMsg->noOfIntegers;
    }

    free(bufptrH);
	//mq_close(mqH);

    // Printing the final result
    for (int i = 0; i < intervalCount; i++) {
        printf("[");
        printf("%d", printArr[i][0]);
        printf(", ");
        printf("%d", printArr[i][1]);
        printf("): ");
        printf("%d", printArr[i][2]);
        printf("\n");
        
    }

    // Alerting the server that the client is terminating
    // mqd_t mq;
    msg.termination = true;
    int y;

    mq = mq_open(MQNAME, O_RDWR);
    if (mq == -1) {
        perror("cannot open msg queue server-client-1\n");
        exit(1);
    }
    printf("mq server-client-1-termination opened, mq id = %d\n", (int) mq);

    y = mq_send(mq, (char *) &msg, sizeof(struct msg), 0);

    if (y == -1) {
        perror("mq_send server-client-1-termination failed\n");
        exit(1);
    }

    //printf("mq_send server-client-1-termination success\n");
    sleep(1);

    mq_close(mq);
    mq_close(mqH);

    return 0;
}