#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "reader_writer.h"


void writer_criticalsection(record* records, int rec, int value, int time, semaphores* sems, double* delayt){
    
    struct timespec start,end;
    clock_gettime(CLOCK_REALTIME, &start);

    sem_wait(&(sems->mutex1));

    sem_wait(&(sems->mutex2));
    if(sems->readact == sems->readcomp){
        sem_post(&(sems->mutex2));                             // Check if any readers are reading
    } else {
        sems->wrtwait = true;
        sem_post(&(sems->mutex2));                            // Release mutex2 before waiting for readers
        sem_wait(&(sems->wrt));                                 // Wait for readers to finish
        sems->wrtwait = false;
    }

    clock_gettime(CLOCK_REALTIME, &end);                        // Critical section

    records[rec-1].value = records[rec-1].value - value;        
    printf("Writing: %d %s %s %d\n",records[rec-1].id,records[rec-1].lastname,records[rec-1].firstname,records[rec-1].value);

    sleep(time);

    sem_post(&(sems->mutex1));

    long d = (end.tv_sec -start.tv_sec) * 1000000000 + (end.tv_sec - start.tv_sec); // Calculate the delay time of the critical section
    *delayt = (double) d / 1000000000.0 ;

}

void reader_criticalsection(record* records, int rec, int time, semaphores* sems, double* delayt){

    struct timespec start,end;
    clock_gettime(CLOCK_REALTIME, &start);

    sem_wait(&(sems->mutex1));
    sems->readact++;
    sem_post(&(sems->mutex1));                                     
    
    clock_gettime(CLOCK_REALTIME, &end);                            // Critical section

    printf("Reading: %d %s %s %d\n",records[rec-1].id,records[rec-1].lastname,records[rec-1].firstname,records[rec-1].value);
    
    sleep(time);    

    sem_wait(&(sems->mutex2));
    sems->readcomp++;

    if(sems->wrtwait && sems->readact == sems->readcomp){             // Last reader signals writers
        sem_post(&(sems->wrt));
    }

    sem_post(&(sems->mutex2));

    long d = (end.tv_sec -start.tv_sec) * 1000000000 + (end.tv_sec - start.tv_sec); // Calculate the delay time of critical section
    *delayt = (double) d / 1000000000.0 ;

}

void update_file(const char* infile, record* rec, int i){       // Function that updates the file with the changed record
    FILE* file = fopen(infile, "rb+");

    if(file == NULL){
        perror("Error opening the file");
        exit(EXIT_FAILURE);
    }

    fseek(file, (i-1)*sizeof(record), SEEK_SET);            // Find record i into the file

    if( fwrite(rec, sizeof(record), 1, file) != 1){         // Write into the file
        perror("Error writing to the file");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fclose(file);
}


record* get_records(char* input_file, int* numrecords){     // Function that reads records from a binary file
    int fd = open(input_file, O_RDONLY);
    if(fd==-1){
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    off_t file_size = lseek(fd,0,SEEK_END);                 // Get the size of file 
    lseek(fd,0,SEEK_SET);
    *numrecords = file_size/sizeof(record);                 // Calculate number of records

    record* records =(record*) malloc (file_size);
    if( records==NULL ){
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read = read(fd,records,file_size);
    if( bytes_read==-1 ){
        perror("Error reading file");
        free(records);
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
    return records;
}

int notinarray(int i, int* array, int size){                // Function that checks if element i is not in array   
    for(int j=0; j<size; j++){
        if (array[j] == i){
            return 0;
        }
    }
    return 1;
}

void printrecords(const char* infile){         // Function that prints records
    FILE* file = fopen(infile, "rb");
    if(file == NULL){
        perror("Error opening the file for reading");
        exit(EXIT_FAILURE);
    } 
    record r;
    while (fread(&r, sizeof(record), 1, file) == 1){
        printf("%d %s %s %d\n",r.id,r.lastname,r.firstname,r.value);
    }
    fclose(file);
}