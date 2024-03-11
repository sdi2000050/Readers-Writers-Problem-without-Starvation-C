#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include "reader_writer.h"

#define MAXPROCESSES 15
#define MAXRECS 8
#define MAXTIME 3

int main(int argc, char *argv[]){

    if (argc != 2){
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char* input_file = argv[1];

    int numrecords;
    record* records = get_records(input_file,&numrecords);

    int shmid;
    shared_data* shd;
    size_t size = 3 * sizeof(int) + numrecords * sizeof(semaphores);

    shmid = shmget(IPC_PRIVATE, size, 0666);                                // Create shared memory
    if (shmid == -1){
        perror("Creation of shared memory failed");
        exit(EXIT_FAILURE);
    }

    shd = (shared_data*) shmat(shmid,NULL,0);
    if (shd == (shared_data*)-1){
        perror("Attachment of shared memory failed");
        exit(EXIT_FAILURE);
    }
   
    shd->numreaders = 0;                                                    // Initialize shared data
    shd->numwriters = 0;
    shd->numrecs = 0;
    shd->readtime = 0;
    shd->wrttime = 0;
    shd->maxdelay = 0;

    
    printf("Initial records:\n");
    printrecords(input_file);
    printf("\n");

    for(int i=0; i<numrecords; i++){                                        // Initialize semaphores for each record
        sem_init(&(shd->record_sem[i].mutex1),1,1);
        sem_init(&(shd->record_sem[i].mutex2),1,1);
        sem_init(&(shd->record_sem[i].wrt),1,0);
        shd->record_sem[i].readcomp = 0;
        shd->record_sem[i].readact = 0;
        shd->record_sem[i].wrtwait = false;
    }   

    srand(time(NULL));
    int numprocesses = (rand() % MAXPROCESSES) + 1;
    char* process[numprocesses];

    /*char comline[1000];
    process[0] = (char*) malloc (1000 * sizeof(char));                     // Code to test the maximum delay
    process[1] = (char*) malloc (1000 * sizeof(char));
    sprintf(comline,"./reader -f accounts50.bin -l 24 -d 10 -s 0");
    strcpy(process[0],comline);
    sprintf(comline,"./writer -f accounts50.bin -l 24 -v 47 -d 1 -s 0");
    strcpy(process[1],comline);*/

    for(int i=0; i<numprocesses; i++){                                  // Create random command lines for readers and writers
        int rw = (rand() % numrecords) + 1;
        process[i] = (char*) malloc (1000 * sizeof(char));
        if( (rw % 2) == 0 ){                                            // Reader
            int numofrecs = (rand() % MAXRECS) + 1;
            int recs[numofrecs];
            int j=0;
            while ( j<numofrecs ){
                int rec = rand() % numrecords + 1;                      // Generate a random number of records and create the command line for the reader
                if ( notinarray(rec,recs,j) ){
                    recs[j] = rec;
                    j++;
                }
            }
            char comline[1000];
            snprintf(comline, sizeof(comline) , "./reader -f %s -l ",input_file);
            for(j=0; j<numofrecs; j++){
                snprintf(comline + strlen(comline), sizeof(comline) - strlen(comline), "%d,",recs[j]);
            }
            int time =  (rand() % MAXTIME) + 1;
            snprintf(comline + strlen(comline), sizeof(comline) - strlen(comline), " -d %d -s %d",time,shmid);

            strcpy(process[i],comline);
        } else {                                                        // Writer
            int rec = (rand() % numrecords) + 1;

            char comline[1000];
            int time = (rand() % MAXTIME) + 1;
            int value = (rand() % records[rec-1].value);
            snprintf(comline, sizeof(comline) , "./writer -f %s -l %d -v %d -d %d -s %d",input_file,rec,value,time,shmid);

            strcpy(process[i],comline);   
        }
    }

    for(int i=0; i<numprocesses; i++){
        pid_t pid;
        pid = fork();
        if( pid == 0 ){                                                 // Create the processes
            char* args[20];                                             // Genarate the command line arguments for the exec
            int j=0;
            char* token = strtok(process[i]," ");
            while (token != NULL){
                args[j++] = token;
                token = strtok(NULL, " ");
            }
            args[j]=NULL;

            execvp(args[0],args);

            perror("Exec failed");
            exit(EXIT_FAILURE);
        }
    }

    for(int i=0; i<numprocesses; i++){
        int status;
        if (waitpid(-1,&status,0) == -1){
            perror("Waitpid failed");
            exit(EXIT_FAILURE);
        }
    }

    printf("\nFinal records:\n");
    printrecords(input_file);
    printf("\nReaders that worked with file: %d\n",shd->numreaders);
    if( shd->numreaders != 0 ){
        printf("Average time that readers worked: %f seconds\n",(shd->readtime /(double)shd->numreaders));
    } else {
        printf("Average time that readers worked: 0.000000 seconds\n");
    }
    printf("Writers that worked with file: %d\n",shd->numwriters);
    if( shd->numwriters != 0 ){
        printf("Average time that writers worked: %f seconds\n",(shd->wrttime /(double)shd->numwriters));
    } else {
        printf("Average time that writers worked: 0.000000 seconds\n");
    }
    printf("Maximum delay from reader/writer: %f seconds\n",shd->maxdelay);
    printf("Total number of used records: %d\n",shd->numrecs);

    
    for(int i=0; i<numrecords; i++){                                        // Initialize semaphores for each record
        sem_destroy(&(shd->record_sem[i].mutex1));
        sem_destroy(&(shd->record_sem[i].mutex2));
        sem_destroy(&(shd->record_sem[i].wrt));
    }
    
    int err;                                                            // Detach and remove shared memory
    err = shmdt(shd);
    if( err == -1 ){
        perror("Detachment of shared memory failed");
        exit(EXIT_FAILURE);
    }
    err = shmctl(shmid, IPC_RMID, 0);
    if( err == -1 ){
        perror("Removal of shared memory failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}
