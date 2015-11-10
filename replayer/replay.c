#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// compile: gcc concurrent.c -pthread

// CONFIGURATION PART
const int BLOCK_SIZE = 512; //in bytes
int numworkers = 1;
char tracefile[] = "trace1.txt";
int printlatency = 0; //print every io latency

// ANOTHER GLOBAL VARIABLES
int fd;
int totalio;
static int jobtracker = 0;
void *buff;
long *blkno; //TODO: devise better way to save blkno,size,flag
int *reqsize;
int *reqflag;

/*=============================================================*/

int readTrace(char ***req){
    //first, read the number of lines
    FILE *trace = fopen(tracefile,"r");
    int ch;
    int numlines = 0;
    
    while(!feof(trace)){
        ch = fgetc(trace);
        if(ch == '\n'){
            numlines++;
        }
    }
    
    rewind(trace);

    //then, start parsing
    if((*req = malloc(numlines * sizeof(char*))) == NULL){
        fprintf(stderr,"Error in memory allocation\n");
    }
    
    char line[100]; //assume it will not exceed 100 chars
    int i = 0;
    while(fgets(line, sizeof(line), trace) != NULL){
        line[strlen(line) - 1] = '\0';
        (*req)[i] = malloc(strlen(line) * sizeof(char));
        strcpy((*req)[i],line);
        i++;
    }
    
    fclose(trace);
    
    return numlines;
}

void arrangeIO(char **requestarray){
    blkno = malloc(totalio * sizeof(long));
    reqsize = malloc(totalio * sizeof(int));
    reqflag = malloc(totalio * sizeof(int));
    
    int i = 0;
    for(i = 0; i < totalio; i++){
        char *io = malloc(strlen(requestarray[i]) * sizeof(char));
        strcpy(io,requestarray[i]);
        
        strtok(io," "); //1. request arrival time
        strtok(NULL," "); //2. device number
        blkno[i] = atoi(strtok(NULL," ")); //3. block number
        reqsize[i] = atoi(strtok(NULL," ")); //4. request size
        reqflag[i] = atoi(strtok(NULL," ")); //5. request flags
    }
}

void *performIO(){
    double sum = 0;
    int howmany = 0;
    int curtask;
    struct timeval t1,t2;

    int i;
    
    while(jobtracker < totalio){
        //firstly save the task to avoid any possible contention later
        curtask = jobtracker; 
        jobtracker++;
        howmany++;    
        
        gettimeofday(&t1,NULL);
        //printf("%lu %d %d\n",blkno[curtask],reqsize[curtask],reqflag[curtask]);
        //do the job
        if(reqflag[curtask] == 0){
            if(pwrite(fd, buff, reqsize[curtask], blkno[curtask]) < 0){
                fprintf(stderr,"Cannot write!\n");
                exit(1);
            }
        }else{
            if(pread(fd, buff, reqsize[curtask], blkno[curtask) < 0){
                fprintf(stderr,"Cannot Read!\n");
                exit(1);
            }
        }
        gettimeofday(&t2,NULL);  
        sum += (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0; 
    }
    
    printf("Average time: %f ms\n", (sum/howmany));
}

void operateWorkers(){
    struct timeval t1,t2;
    float totaltime;
    
    // thread creation
    pthread_t *tid = malloc(numworkers * sizeof(pthread_t));
    int x;
    gettimeofday(&t1,NULL);
    for(x = 0; x < numworkers; x++){
        pthread_create(&tid[x], NULL, performIO, NULL);
    }
    for(x = 0; x < numworkers; x++){
        pthread_join(tid[x], NULL);
    }
    gettimeofday(&t2,NULL);
    totaltime = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
    printf("Total run time: %.3f ms\n",totaltime);
}

int main(int argc, char *argv[]) {
    char device[64];
    char **request;
    
    if (argc <= 1){
        printf("Please specify device name\n");
        exit(1);
    }else{
        sprintf(device,"%s",argv[1]);
    }
    
    // read the trace before everything else
    totalio = readTrace(&request);
    arrangeIO(request);
    
    // start the disk part
    fd = open(device, O_DIRECT | O_SYNC | O_RDWR);
    if(fd < 0) {
        fprintf(stderr,"Cannot open %s\n", device);
        exit(1);
    }
    
    if (posix_memalign(&buff,4096,4096)){
        fprintf(stderr,"memory allocation failed\n");
        exit(1);
    }
    
    operateWorkers();

    return 0;
}
