#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <time.h>
#include "reader_writer.h"

int main(int argc, char *argv[]){

    struct timespec start,end;
    clock_gettime(CLOCK_REALTIME, &start);

    if (argc != 11) {                                    // Check if the correct number of command line arguments is provided
        fprintf(stderr, "Usage: %s -f filename -l recid -v value -d time -s shmid\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char* input_file = NULL;                            // Parse command line arguments
    int rec;
    int value;
    int time;
    int shmid;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 < argc) {
                input_file = argv[i + 1];
                i++; 
            } else {
                fprintf(stderr, "Missing value for -f\n");
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "-l") == 0) {
            if (i + 1 < argc) {
                rec = atoi(argv[i + 1]);
                i++; 
            } else {
                fprintf(stderr, "Missing value for -l\n");
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "-v") == 0) {
            if (i + 1 < argc) {
                value = atoi(argv[i + 1]);
                i++; 
            } else {
                fprintf(stderr, "Missing value for -v\n");
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "-d") == 0){
            if (i + 1 < argc) {
                time = atoi(argv[i + 1]);
                i++; 
            } else {
                fprintf(stderr, "Missing value for -d\n");
                exit(EXIT_FAILURE);
            }    
        } else if (strcmp(argv[i], "-s") == 0){
            if (i + 1 < argc) {
                shmid = atoi(argv[i + 1]);
                i++; 
            } else {
                fprintf(stderr, "Missing value for -s\n");
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "Invalid option: %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }
    printf("%s -f %s -l %d -v %d -d %d -s %d\n",argv[0],input_file,rec,value,time,shmid);

    shared_data* shd = (shared_data*) shmat (shmid,NULL,0);         // Atach shared memory
    if( shd == (shared_data*) -1 ){
        perror("Writers atachment of shared memory failed");
        exit(EXIT_FAILURE);
    }

    int numrecords;
    record* records = get_records(input_file,&numrecords);

    shd->numwriters++;                                              // Update shared memory data
    shd->numrecs++;
    double delayt;

    writer_criticalsection(records,rec,value,time,&shd->record_sem[rec-1],&delayt);

    update_file(input_file,&records[rec-1],rec);

    if(shd->maxdelay < delayt){
        shd->maxdelay = delayt;
    }

    clock_gettime(CLOCK_REALTIME, &end);                            // Calculate total time of working writer

    long d = (end.tv_sec -start.tv_sec) * 1000000000 + (end.tv_sec - start.tv_sec);
    double acttime = (double) d / 1000000000.0 ;

    shd->wrttime += acttime;

    int err;
    err = shmdt(shd);
    if( err == -1 ){
        perror("Writers detachment of shared memory failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}