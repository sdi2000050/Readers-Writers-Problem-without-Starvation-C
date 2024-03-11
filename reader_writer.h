#include <stdbool.h>
#include <semaphore.h>

typedef struct record{
    int id;
    char lastname[20];
    char firstname[20];
    int value;
}record;

typedef struct semaphores {
    sem_t mutex1;
    sem_t mutex2;
    sem_t wrt;

    int readcomp;           // Completed readers
    int readact;            // Active readers
    bool wrtwait;           // Flag for waiting writers
}semaphores;

typedef struct shared_data {                    // Shared data for statistics and semaphores
    int numreaders;                             
    int numwriters;
    int numrecs;
    double readtime;
    double wrttime;
    double maxdelay;

    semaphores record_sem[100000];              // Array of semaphores for each record (assumimg max 100000 records)
}shared_data;

record* get_records(char* input_file, int* numrecords);
void update_file(const char* infile, record* record, int rec);
int notinarray(int i, int* array, int size);
void printrecords(const char* infile);

void writer_criticalsection(record* records, int rec, int value, int time, semaphores* sems, double* delayt);
void reader_criticalsection(record* records, int rec, int time, semaphores* sems, double* delayt);
