#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <linux/types.h>
#include <linux/magic.h>
#include <asm/byteorder.h>
#include "/usr/include/linux/msdos_fs.h"

#define SECTORSIZE 512
#define CLUSTERSIZE 1024

// Global Variables
char *DISKIMAGE;
unsigned char sector[SECTORSIZE];
unsigned char cluster[CLUSTERSIZE];
unsigned int data_start_sector;
struct fat_boot_sector *bp;
unsigned char num_fats, sectors_per_cluster;
unsigned int num_sectors, sectors_per_fat;
unsigned int root_start_cluster;
int cluster_count;

int fd;

// Start of  Helper Methods

int readsector (int fd, unsigned char *buf, unsigned int snum) {
    off_t offset;
    int n;
    offset = snum * SECTORSIZE;
    lseek (fd, offset, SEEK_SET);
    n = read (fd, buf, SECTORSIZE);
    if (n == SECTORSIZE)
    return (0);
    else
    return(-1);
}

int writesector (int fd, unsigned char *buf, unsigned int snum) {
    off_t offset;
    int n;
    offset = snum * SECTORSIZE;
    lseek (fd, offset, SEEK_SET);
    n = write (fd, buf, SECTORSIZE);
    fsync (fd); // write without delayed-writing
    if (n == SECTORSIZE)
    return (0); // success
    else
    return(-1);
}

int readcluster (int fd, unsigned char *buf, unsigned int cnum) {
    off_t offset;
    int n;
    unsigned int snum; // sector number
    snum = data_start_sector + (cnum - 2) * sectors_per_cluster;
    offset = snum * SECTORSIZE;
    lseek (fd, offset, SEEK_SET);
    n = read (fd, buf, CLUSTERSIZE);
    if (n == CLUSTERSIZE)
        return (0); // success
    else
        return (-1);
}

// Main Function

int main(int argc, char *argv[]) {

    //---------------------------------------------------------
    sectors_per_cluster = 2;


    //-----------------------------------------------------------
    
    fd = open (argv[1], O_SYNC | O_RDONLY); // disk fd

    char *c = argv[2];

    if (strcmp(c, "-v") == 0)
        printV();
    if (strcmp(c, "-s") == 0)
        printS(argv[3]);
    if (strcmp(c, "-c") == 0)
        printC(argv[3]);
    if (strcmp(c, "-t") == 0)
        printT();
    if (strcmp(c, "-r") == 0)
        printR(argv[3], argv[4], argv[5]);
    if (strcmp(c, "-b") == 0)
        printB(argv[3]);
    if (strcmp(c, "-a") == 0)
        printA(argv[3]);
    if (strcmp(c, "-n") == 0)
        printN(argv[3]);
    if (strcmp(c, "-m") == 0)
        printM(argv[3]);
    if (strcmp(c, "-f") == 0)
        printF(argv[3]);
    if (strcmp(c, "-d") == 0)
        printD(argv[3]);
    if (strcmp(c, "-l") == 0)
        printL(argv[3]);
    if (strcmp(c, "-h") == 0)
        printH();
}

// Print Functions

void printV() {

    readsector (fd, sector, 0); // read sector #0
    readcluster(fd, cluster, 32);
    bp = (struct fat_boot_sector *) sector; // type casting

    unsigned int sectors_per_fat = bp->fat32.length;
    data_start_sector = 32 + 2 * sectors_per_fat;

    struct msdos_dir_entry *dep;
    dep = (struct msdos_dir_entry *) cluster;

    int cluster_count = (sectors_per_fat * 512 / 4);
    unsigned char vol_label[11];
    for (int i = 0; i < 11; i++)
    {
        vol_label[i] = bp->fat32.vol_label[i];
    }

    int bytes_per_sector = 512;

    printf("File system type: FAT32\n");
    printf("Volume label: %s\n", vol_label);
    printf("Number of sectors in disk: %d\n", bp->total_sect);
    //printf("Sector size in bytes: %02lx%02lx\n", sector[12], sector[11]);
    printf("Sector size in bytes: %d\n", bytes_per_sector);
    printf("Number of reserved sectors: %d\n", bp->reserved);
    printf("Number of sectors per FAT table: %d\n", sectors_per_fat);
    printf("Number of FAT tables: %d\n", bp->fats);
    printf("Number of sectors per cluster: %d\n", bp->sec_per_clus);
    printf("Number of clusters: %d\n", cluster_count);
    printf("Data region starts at sector: %d\n", data_start_sector);
    printf("Root directory starts at sector: %d\n", data_start_sector);
    printf("Root directory starts at cluster: %d\n", bp->fat32.root_cluster);
    printf("Disk size in bytes: %d bytes\n", bytes_per_sector * bp->total_sect);
    printf("Disk size in Megabytes: %d MB\n", (bytes_per_sector * bp->total_sect) / 1048576);

    int freeCount = 0;
    int allocCount = 0;
    
    for (int i = 32; i < sectors_per_fat + 32; i++) {
        readsector (fd, sector, i); // read sector #i
        int bts = 0;
        unsigned int result;
        for (int k = 0; k < 128; k++) {
            result = (sector[bts] << 24) | (sector[bts+1] << 16) | (sector[bts+2] << 8) | (sector[bts+3]);
            bts = bts + 4;
            //printf("%d ", result);
            if (result == 0)
                freeCount++;
            else
                allocCount++;
        }
    }

    printf("Number of used clusters: %d\n", allocCount);
    printf("Number of free clusters: %d\n", freeCount);
}

void printA(char *path) {

    unsigned char *parse_container;
    int parser = 0;

    unsigned char *ptr = strtok(path, "/");
    while (ptr != NULL) {
        parse_container[parser++] = ptr;
        ptr = strtok(NULL, "/");
    }

    readcluster(fd, cluster, 2);

    struct msdos_dir_entry *dep;
    dep = (struct msdos_dir_entry *) cluster;

    printf("%s\n", dep->name);
    char *temp = dep->name;
    temp[strlen(temp)] = '\0';
    char *temp2 = parse_container[0];
    temp[strlen(temp2)] = '\0';

    printf("%u", parse_container[0]);

    if (strcmp(temp2, temp)) {
       
    }
    else {
        printf("Error! Couldn't find the path.\n");
        return;
    }
}

void printF(char *count) {

    // one sector has 128 entries 512*8/32 = 128
    int noOfSectors;

    if (atoi(count) < 128 && atoi(count) > 0)
        noOfSectors = 1;
    if (atoi(count) > 128)
        noOfSectors = (atoi(count) / 128) + 1;

    int counter = 0;
    int sectorsTraversed = noOfSectors;

    //printf("No of count:%d\n", atoi(count));
    printf("No of sectors:%d\n", noOfSectors);

    if (atoi(count) == -1) {
        noOfSectors = 1016;

        for (int i = 32; i < noOfSectors + 32; i++) {
            readsector (fd, sector, i); // read sector #i
            int bts = 0;
            unsigned int result;

            for (int k = 0; k < 128; k++) {
                result = (sector[bts+3] << 24) | (sector[bts+2] << 16) | (sector[bts+1] << 8) | (sector[bts]);
                bts = bts + 4;
                //printf("%d ", result);
                if (result >= 268435448)
                    printf("%07d: EOF\n", counter);
                else if (result == 268435447)
                    printf("%07d: Bad Cluster\n", counter);
                else
                    printf("%07d: %d\n", counter, result);

                counter++;
            }
        }   
    }

    else {
        for (int i = 32; i < noOfSectors + 32; i++) {
            readsector (fd, sector, i); // read sector #i
            int bts = 0;
            unsigned int result;

            if (sectorsTraversed > 1) {
                for (int k = 0; k < 128; k++) {
                    result = (sector[bts+3] << 24) | (sector[bts+2] << 16) | (sector[bts+1] << 8) | (sector[bts]);
                    bts = bts + 4;
                    //printf("%d ", result);
                    if (result >= 268435448)
                        printf("%07d: EOF\n", counter);
                    else if (result == 268435447)
                        printf("%07d: Bad Cluster\n", counter);
                    else
                        printf("%07d: %d\n", counter, result);

                    counter++;
                }
                sectorsTraversed--;
            }

            if (sectorsTraversed == 1) {
                for (int k = 0; k < atoi(count) % 128; k++) {
                    result = (sector[bts+3] << 24) | (sector[bts+2] << 16) | (sector[bts+1] << 8) | (sector[bts]);
                    bts = bts + 4;
                    //printf("%d ", result);
                    if (result >= 268435448)
                        printf("%07d: EOF\n", counter);
                    else if (result == 268435447)
                        printf("%07d: Bad Cluster\n", counter);
                    else
                        printf("%07d: %d\n", counter, result);

                    counter++;
                }
                sectorsTraversed--;
            }
        }
    }
}

void printC(char *cNum) {

    sectors_per_cluster = 2;
   
    readsector (fd, sector, 0); // read sector #0
    readcluster(fd, cluster, 32);
    bp = (struct fat_boot_sector *) sector; // type casting

    unsigned int sectors_per_fat = bp->fat32.length;
    data_start_sector = 32 + 2 * sectors_per_fat;


    int clusterNum = atoi(cNum);

    if (clusterNum < 2) {
        printf("Cluster does not exist!\n");
        return;
    }

    readcluster (fd, cluster, clusterNum);
    int counter = 0;
    
   for (int i = 0; i < CLUSTERSIZE; i = i + 16) {
        printf("%08x: ", i);
        char a[16] = "";

        for (int k = 0; k < 16; k++) {
            printf("%02lx ", cluster[counter]);

            a[k] = cluster[counter];

            counter++;
        }

        printf("  ");

        
        for (int j = 0; j < 16; j++) {
            if (isprint(a[j]))
                printf("%c", a[j]);
            else
                printf(".");
        }
        

        printf("\n");
    }
    
}

void printT() {

}

void printR(char *path, int offest, int count) {

}

void printB(char *path) {

}

void printN(char *path) {

}

void printM(char *count) {

    sectors_per_cluster = 2;
   
    readsector (fd, sector, 0); // read sector #0
    readcluster(fd, cluster, 32);
    bp = (struct fat_boot_sector *) sector; // type casting

    unsigned int sectors_per_fat = bp->fat32.length;
    data_start_sector = 32 + 2 * sectors_per_fat;

    int noOfClusters = atoi(count);
    int counter = 0;

    //printf("No of count:%d\n", atoi(count));
    //printf("No of sectors:%d\n", noOfClusters);
    struct msdos_dir_entry *dep;

    /*if (atoi(count) == -1) {

        for (int i = 0; i < noOfClusters; i++) {
            readsector (fd, sector, i); // read sector #i
            int bts = 0;
            unsigned int result;

            for (int k = 0; k < 128; k++) {
                result = (sector[bts] << 24) | (sector[bts+1] << 16) | (sector[bts+2] << 8) | (sector[bts+3]);
                bts = bts + 4;
                //printf("%d ", result);
                if (result >= 268435448)
                    printf("%07d: EOF\n", counter);
                else if (result == 268435447)
                    printf("%07d: Bad Cluster\n", counter);
                else
                    printf("%07d: %d\n", counter, result);

                counter++;
            }
        }   
    }*/

    //else {
        for (int i = 0; i < noOfClusters; i++) {
            readcluster (fd, cluster, i); // read sector #i
            dep = (struct msdos_dir_entry *) cluster;
            int bts = 0;
            unsigned int result;

            result = (cluster[bts] << 24) | (cluster[bts+1] << 16) | (cluster[bts+2] << 8) | (cluster[bts+3]);
            bts = bts + 4;
            
            if (result >= 268435448)
                printf("%07d: --EOF--\n", counter);
            else if (result == 268435447)
                printf("%07d: --BAD CLUSTER--\n", counter);
            else if (result == 0)
                printf("%07d: --FREE--\n", counter);
            else
                printf("%07d: %d\n", counter, dep->name);

            counter++;
        }
    //}
}

void printD(char *path) {

}

void printL(char *path) {

}

void printS(char *sNum) {

    int sectorNum = atoi(sNum);

    if (sectorNum < 0) {
        printf("Sector does not exist!\n");
        return;
    }

    readsector (fd, sector, sectorNum);
    int counter = 0;

    for (int i = 0; i < 32; i++) {
        printf("%08x: ", i*16);
        char a[16] = "";

        for (int k = 0; k < 16; k++) {
            printf("%02lx ", sector[counter]);

            a[k] = sector[counter];

            counter++;
        }

        printf("  ");

        for (int j = 0; j < 16; j++) {
            if (isprint(a[j]))
                printf("%c", a[j]);
            else
                printf(".");
        }

        printf("\n");
    }
}

void printH() {

    printf("----------------------------------HELP PAGE----------------------------------\n\n");

    printf("fat DISKIMAGE -v: Prints some summary information about the specified FAT32 volume DISKIMAGE. Example invocation:\n");
    printf("./fat disk1 -v\n\n");

    printf("fat DISKIMAGE -s SECTORNUM: Prints the content (byte sequence) of the specified sector to screen in hex form. Example invocation:\n");
    printf("./fat disk1 -s 32\n\n");

    printf("fat DISKIMAGE -c CLUSTERNUM: Prints the content (byte sequence) of the specified cluster to the screen in hex form. Example invocation:\n");
    printf("./fat disk1 -c 2\n\n");

    printf("fat DISKIMAGE -t: Prints all directories and their files and subdirectories starting from the root directory, recursively, in a depth-first search order. Example invocation:\n");
    printf("./fat disk1 -t\n\n");

    printf("fat DISKIMAGE -a PATH: Prints the content of the ascii text file indicated with PATH to the screen as it is. Example invocation:\n");
    printf("./fat disk1 -a /DIR2/F1.TXT\n\n");

    printf("fat DISKIMAGE -b PATH: Prints the content (byte sequence) of the file indicated with PATH to the screen in hex form. Example invocation:\n");
    printf("./fat disk1 -b /DIR2/F1.TXT\n\n");

    printf("fat DISKIMAGE -l: Prints the names of the files and subdirectories in the directory indicated with PATH. Example invocation(s):\n");
    printf("./fat disk1 -l /\n");
    printf("./fat disk1 -l /DIR2\n\n");

    printf("fat DISKIMAGE -n PATH: Prints the numbers of the clusters storing the content of the file or directory indicated with PATH. Example invocation:\n");
    printf("./fat disk1 -n /DIR1/AFILE1.BIN\n\n");

    printf("fat DISKIMAGE -d PATH: Prints the content of the directory entry of the file or directory indicated with PATH. Example invocation:\n");
    printf("./fat disk1 -d /DIR1/AFILE1.BIN\n\n");

    printf("fat DISKIMAGE -f COUNT: Prints the content of the FAT table up to the specified COUNT. If COUNT is -1, prints out the entire table. Example invocation:\n");
    printf("./fat disk1 -f 50\n\n");

    printf("fat DISKIMAGE -r PATH OFFSET COUNT: Reads COUNT bytes from the file indicated with PATH starting at OFFSET (byte offset into the file) and prints the bytes read to the screen. Example invocation:\n");
    printf("./fat disk1 -r /DIR2/F1.TXT 100 64\n\n");

    printf("fat DISKIMAGE -m COUNT: Prints a map of the volume. If COUNT is -1, prints information about all the clusters. Example invocation:\n");
    printf("./fat disk1 -m 100\n\n");

    printf("fat -h: Prints out this help page. Example invocation:\n");
    printf("./fat disk1 -h\n\n");

    printf("----------------------------------END OF HELP PAGE----------------------------------\n");
}