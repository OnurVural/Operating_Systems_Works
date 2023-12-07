#include "dma.h"
#include <sys/mman.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

/*
    pagesize = 4 kb
    memory management unit is word
    word size = 8 bytes
    minimum size of block is 2 words
    allocation unit will be 2 words
    size of segment will be a power of 2
    segment must start at a page boundary -- use mmap

    bitmap will sit at the offset 0 of the segment
    1 bit for each word of the segment in the bitmap
    free block of n words will be represented with n consecutive 1's
    used block 01 and (n-2) 0's n >= 2
    e.g. free block of 4 words is 1111
    if segment is 2^30 bytes, # of bits in bitmap will be 2^30/2^3=2^27
    after the bitmap, 256 reserved space (see the figure)

    allocated block size will always be multiple of double word size 
    e.g. if 3 bytes requested, 16 bytes will be allocated (2 words)
    we may have both internal and external fragmentation

    to allocate, search from left to right to find large enough block

    ! thread-safe !

*/

// Global Variables
char bitmap[4194304]; // 2^22 = 4194304
int segmentSize;
int noOfBits;
void *segmentPointer;
void *startOfBitmap;
void *startOfReservedSpace;
void *startOfAllocation;

int noOfBitsReserve;
int noOfWordsBitmap;

int internalFrag = 0;

pthread_mutex_t m1;
pthread_mutex_t m2;
pthread_mutex_t m3;
pthread_mutex_t m4;
pthread_mutex_t m5;
pthread_mutex_t m6;


int dma_init (int m) {

    pthread_mutex_init(&m1, NULL);
    pthread_mutex_init(&m2, NULL);
    pthread_mutex_init(&m3, NULL);
    pthread_mutex_init(&m4, NULL);
    pthread_mutex_init(&m5, NULL);
    pthread_mutex_init(&m6, NULL);

    if (m >= 14 && m <= 22) {
        
        segmentSize = (int) (pow(2.00, (double)m));
        segmentPointer = mmap (NULL, (size_t) segmentSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (segmentPointer == MAP_FAILED) {
            printf("mmap() failed to create!\n");
            return -1;
        }

        printf("%lx\n", (long) segmentPointer); // print start address

        //while (1)
            //; // loop so that we can run pmap on the process

        // bitmap init
        noOfBits = (int) (pow(2.00, (double)m - 3));

        //printf("%d\n", noOfBits); // print start address


        for (int i = 0; i < noOfBits; i++) {
            bitmap[i] = '1';
        }

        // now we encode bitmap as used space in bitmap
        int divider = 8 * 8; // since one byte is 8 bits and word size is 8 bytes
        noOfWordsBitmap = noOfBits / divider;

        for (int i = 0; i < noOfWordsBitmap; i++) {
            bitmap[i] = '0';
            if (i == 1)
                bitmap[i] = '1';
        }

        // reserved space init - encode reserved space as used in bitmap
        noOfBitsReserve = 256 / 8; // reserve is 256 bytes, word size is 8 bytes

        for (int i = noOfWordsBitmap; i < noOfWordsBitmap + noOfBitsReserve; i++) {
            bitmap[i] = '0';
            if (i == 1 + noOfWordsBitmap)
                bitmap[i] = '1';
        }

        // setting checkpoints
        startOfBitmap = segmentPointer;
        startOfReservedSpace = startOfBitmap + (noOfBits / 8);
        startOfAllocation = startOfReservedSpace + (noOfBitsReserve * 8);

        //printf("%p, %p, %p", startOfBitmap, startOfReservedSpace, startOfAllocation);
    }

    else {
        printf("Segment size is not in the desired interval. Init failed!\n");
        return -1;
    }

    return 0;
}

void *dma_alloc (int size) {

    //printf("debug\n");
    pthread_mutex_lock(&m1);
    //printf("debug\n");

    // find required allocation space
    int requiredSpace;

    if (size < 2) {
        printf("Size must be greater or equal than 2!\n");
        pthread_mutex_unlock(&m1);
        return NULL;
    }

    if (size % 16 == 0) {
        requiredSpace = size;
    }

    if (size < 16) {
        requiredSpace = 16;
        internalFrag = internalFrag + 16 - size;
    }

    if (size > 16 && size % 16 != 0) {
        int result = size / 16;
        requiredSpace = 16 * (result + 1);
        internalFrag = internalFrag + requiredSpace - size;
    }

    // checking space requirement
    if (requiredSpace > segmentSize) {
        printf("Error, size too big for segment!\n");
        pthread_mutex_unlock(&m1);
        return NULL;
    }

    void *startOfNewAlloc;
    int i = 0;

    void *temp = segmentPointer;

    // traverse the bitmap
    while (i < noOfBits) { //for (int i = 0; i < noOfBits - i; i = i + 2) {      // 0100 01 01 1111 01
        int count = 0;

        //printf("debug\n");

        if (bitmap[i] == '1' && bitmap[i+1] == '1') {

            temp = temp + (i * 8);

            for (int k = i; k < noOfBits; k++) {
                if (bitmap[k] != '0') {
                    count++;
                }
                else
                    break;
            }
        }

        // space not big enough
        if (count * 8 < requiredSpace && bitmap[i] == '1' && bitmap[i+1] == '1') {
            i = i + count;
            continue;
        }

        // space big enough, do the allocation      // 0100 01 01 0100 01
        if (count * 8 >= requiredSpace) {
            //printf("Count: %d\n", count);
            bitmap[i] = '0';

            int end = i + (requiredSpace / 8);

            //printf("required space: %d\n", requiredSpace);
            
            for (int j = i + 2; j < end; j++) {
                bitmap[j] = '0';
            }

            startOfNewAlloc = temp; // since i increments in bits and we need bytes
            pthread_mutex_unlock(&m1);
            return startOfNewAlloc;
        }

        i = i + 2;
    }

    //pthread_mutex_unlock(&m1);

    // if we reach the end of loop, we have traversed the entire bitmap, and couldn't find a block big enough
    printf("Error, could not find a block that is available and big enough for allocation!\n");
    pthread_mutex_unlock(&m1);
    return NULL;
}

void dma_free (void *p) {

    pthread_mutex_lock(&m2);

    void *difference = p - startOfAllocation;
    // we assume they are 8 bytes long, so in bitmap, each one corresponds to one bit

    //printf("%p\n", p);
    //printf("%p\n", startOfAllocation);
    //printf("%p\n", difference);

    unsigned long int temp1 = (unsigned long int)difference; //0x7eff27512200
    //printf("%lu\n", temp1);


    // start of this block is this        // 0100 01 01 1111 01
    int address = (int)(temp1 / 8) + noOfBitsReserve + noOfWordsBitmap;
    bitmap[address] = '1';

    for (int i = 0; i < noOfBits; i++) {

        if (i == address + 2) {

            for (int k = i; k < noOfBits; k = k + 2) {
                if (bitmap[k] == '0' && bitmap[k + 1] != '1') {
                    bitmap[k] = '1';
                    bitmap[k + 1] = '1';
                }
                else
                    break;
            }
        }
    }

    pthread_mutex_unlock(&m2);
}

void dma_print_page(int pno) {

    pthread_mutex_lock(&m3);

    // [0, SS/PS), where SS is segment size and PS is the pagesize.

    int pageSize = 4096; // pagesize is 4 KB = 4096 bytes
    int validInterval = segmentSize / pageSize;
    //int length = validInterval * 8;

    if (pno < 0 || pno > validInterval) {
        printf("Error! pno is not in the interval.\n");
        pthread_mutex_unlock(&m3);
        return;
    }

    char* iterator = (char *)segmentPointer;
    iterator = iterator + (pno * 4096);
    const int pageSizeBits = 4096 * 8;
    int counter = 1;

    for (int i = 0; i < pageSizeBits; i = i + 4) {

        int b1 = (iterator[i/8] & (1 << (i%8))) != 0;
        int b2 = (iterator[(i+1)/8] & (1 << ((i+1)%8))) != 0;
        int b3 = (iterator[(i+2)/8] & (1 << ((i+2)%8))) != 0;
        int b4 = (iterator[(i+3)/8] & (1 << ((i+3)%8))) != 0;

        b1 = b1 * pow(2, 3);
        b2 = b2 * pow(2, 2);
        b3 = b3 * pow(2, 1);
        b4 = b4 * pow(2, 0);

        int sum = b1 + b2 + b3 + b4;

        printf("%x", sum);

        if (counter % 64 == 0)
            printf("\n");

        counter++;
    }

    printf("\n");

    pthread_mutex_unlock(&m3);
}

void dma_print_bitmap() {

    pthread_mutex_lock(&m4);

    int charCount = 0;
    int lineCount = 0;

    for (int i = 0; i < noOfBits; i++) {
        if (charCount % 8 == 0) {
            printf(" ");
        }

        if (lineCount % 64 == 0) {
            printf("\n");
        }

        printf("%c", bitmap[i]);
        charCount++;
        lineCount++;
    }

    printf("\n");

    pthread_mutex_unlock(&m4);
}

void dma_print_blocks() {

    pthread_mutex_lock(&m5);

    void *temp  = segmentPointer;

    int i = 0;
    
    while (i < noOfBits) { //for (int i = 0; i < noOfBits; i = i + 2) {

        int count = 0;

        // if to check free block
        if (bitmap[i] == '1' && bitmap[i+1] == '1') {

            for (int k = i; k < noOfBits; k++) {
                if (bitmap[k] != '0') {
                    count++;
                }
                else
                    break;
            }
            printf("%c, %p, 0x%x, (%d)\n", 'F', temp, count * 8, count * 8);
            temp = temp + (count * 8);
            i = i + count;
            //printf("%p\n", temp);
            continue;
        }

        // if to check allocated block
        if (bitmap[i] == '0' && bitmap[i+1] == '1') {

            count = 2;

            for (int k = i + 2; k < noOfBits; k = k + 2) {
                if (bitmap[k] == '0' && bitmap[k + 1] != '1') {
                    count = count + 2;
                }
                else 
                   break;
            }
            printf("%c, %p, 0x%x, (%d)\n", 'A', temp, count * 8, count * 8);
            temp = temp + (count * 8);
            i = i + count;
            //printf("%p\n", temp);
            continue;
        }

        i = i + 2;
    }
    printf("\n");
    pthread_mutex_unlock(&m5);
}

int dma_give_intfrag() {

    pthread_mutex_lock(&m6);
    pthread_mutex_unlock(&m6);
    return internalFrag;
}