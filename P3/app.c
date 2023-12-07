#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "dma.h" // include the library interface

#include <sys/time.h>

int main (int argc, char ** argv) {

    struct timeval t0; // start time
    struct timeval t1; // end time
    long elapsed;  

     void *p1;
     void *p2;
     void *p3;
     void *p4;
     void *p5;
     void *p6;
     void *p7;
     
     int ret;

     ret = dma_init (20); // create a segment of 1 MB

     if (ret != 0) {
     printf ("something was wrong\n");
     exit(1);
     }

    // GRAPH 1: ALLOCATION ----------------------------------------------------------------------------------------
     gettimeofday(&t0, 0);
     p1 = dma_alloc (16); // allocate space for 16 bytes
     gettimeofday(&t1, 0);
     elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;   
     printf("Time for allocation of 16 bytes\n");
     printf("microseconds : %ld\n", elapsed);

          gettimeofday(&t0, 0);
     dma_free(p1); // deallocate 16 bytes
     gettimeofday(&t1, 0);
     elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;   
     printf("Time for deallocation of 16 bytes\n");
     printf("microseconds : %ld\n", elapsed);


     gettimeofday(&t0, 0);
     p2 = dma_alloc (32); // allocate space for 32 bytes
     gettimeofday(&t1, 0);
     elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;   
     printf("Time for allocation of 32 bytes\n");
     printf("microseconds : %ld\n", elapsed);

          gettimeofday(&t0, 0);
     dma_free(p2); // deallocate 32 bytes
     gettimeofday(&t1, 0);
     elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;   
     printf("Time for deallocation of 32 bytes\n");
     printf("microseconds : %ld\n", elapsed);

     gettimeofday(&t0, 0);
     p3 = dma_alloc (64); // allocate space for 64 bytes
     gettimeofday(&t1, 0);
     elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;   
     printf("Time for allocation of 64 bytes\n");
     printf("microseconds : %ld\n", elapsed);


     gettimeofday(&t0, 0);
     dma_free(p3); // deallocate 64 bytes
     gettimeofday(&t1, 0);
     elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;   
     printf("Time for deallocation of 64 bytes\n");
     printf("microseconds : %ld\n", elapsed);


     gettimeofday(&t0, 0);
     p4 = dma_alloc (128); // allocate space for 128 bytes
     gettimeofday(&t1, 0);
     elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;   
     printf("Time for allocation of 128 bytes\n");
     printf("microseconds : %ld\n", elapsed);


     gettimeofday(&t0, 0);
     dma_free(p4); // deallocate 128 bytes
     gettimeofday(&t1, 0);
     elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;   
     printf("Time for deallocation of 128 bytes\n");
     printf("microseconds : %ld\n", elapsed);


     gettimeofday(&t0, 0);
     p5 = dma_alloc (256); // allocate space for 256 bytes
     gettimeofday(&t1, 0);
     elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;   
     printf("Time for allocation of 256 bytes\n");
     printf("microseconds : %ld\n", elapsed);

          gettimeofday(&t0, 0);
     dma_free(p5); // deallocate 256 bytes
     gettimeofday(&t1, 0);
     elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;   
     printf("Time for deallocation of 256 bytes\n");
     printf("microseconds : %ld\n", elapsed);


     gettimeofday(&t0, 0);
     p6 = dma_alloc (512); // allocate space for 512 bytes
     gettimeofday(&t1, 0);
     elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;   
     printf("Time for allocation of 512 bytes\n");
     printf("microseconds : %ld\n", elapsed);

          gettimeofday(&t0, 0);
     dma_free(p6); // deallocate 512 bytes
     gettimeofday(&t1, 0);
     elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;   
     printf("Time for deallocation of 512 bytes\n");
     printf("microseconds : %ld\n", elapsed);


     gettimeofday(&t0, 0);
     p7 = dma_alloc (1024); // allocate space for 1024 bytes
     gettimeofday(&t1, 0);
     elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;   
     printf("Time for allocation of 1024 bytes\n");
     printf("microseconds : %ld\n", elapsed);

          gettimeofday(&t0, 0);
     dma_free(p7); // deallocate 1024 bytes
     gettimeofday(&t1, 0);
     elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;   
     printf("Time for deallocation of 1024 bytes\n");
     printf("microseconds : %ld\n", elapsed);
     // GRAPH 1 END----------------------------------------------------------------------------------------
     
    // GRAPH 2: DEALLOCATION ----------------------------------------------------------------------------------------

    
    // GRAPH 2 END----------------------------------------------------------------------------------------
     
    // GRAPH 3: Random Alloc, Internal and External FRAGMENTATION----------------------------------------------------------------------------------------
    void *r1;
    /*
    void *r2;
    void *r3;
    void *r4;
    void *r5;
    void *r6;
    void *r7;
    void *r8;
    void *r9;
    void *r10;
    */

    int random; 
    random = rand() % 800000; // 0.8 MB??? sınır belirlemek lazım

    /*
    gettimeofday(&t0, 0);
    r1 = dma_alloc(random); 
    gettimeofday(&t1, 0);
    elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;   
    printf("microseconds : %d\n", elapsed);
    if( !r1 ){
        printf("allocation CANNOT be done");    
    }
    else {
       printf("allocation of bytes: %ld\n", random);
    }
    */

    int intF = dma_give_intfrag();

    for (int i = 0; i < 10; i++){
        random = rand() % 800000;
        gettimeofday(&t0, 0);
        r1 = dma_alloc(random); 
        gettimeofday(&t1, 0);
        elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;   
        printf("microseconds : %d\n", elapsed);

        int intF = dma_give_intfrag();
        printf("TOTAL internal fragmentation: %d\n", intF);
        
        if( !r1 ){
            printf("allocation CANNOT be done");    
         }
        else {
            printf("allocation of bytes: %ld\n", random);
            
        }

    }
    
    // GRAPH 3 END----------------------------------------------------------------------------------------
     return 0;
}