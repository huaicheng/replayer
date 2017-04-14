#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <inttypes.h>
#include <linux/fs.h>
#include <sys/ioctl.h>

// compile: gcc replay.c -pthread

//Note: all sizes are in block (1 block = block_size bytes)

// CONFIGURATION PART


int LARGEST_REQUEST_SIZE = 65536; //blocks
int MEM_ALIGN = 4096*8; //bytes
int numworkers = 1; // =number of threads
int printlatency = 0; //print every io latency
int maxio = 5000000; //halt if number of IO > maxio, to prevent printing too many to metrics file
int respecttime = 0;
int check_cache = 1;
int block_size = 512; 
int64_t DISK_SIZE = 0;

// ANOTHER GLOBAL VARIABLES
int fd;
int totalio;
int jobtracker = 0;
int latecount = 0;
int slackcount = 0;
void *buff;
uint64_t starttime;

int64_t *blkno; //TODO: devise better way to save blkno,size,flag
int *reqsize;
int *reqflag;
float *timestamp;

FILE *metrics; //current format: offset,size,type,latency(ms)

pthread_mutex_t lock;

/*=============================================================*/

int64_t get_disksz(int devfd)
{
    int64_t sz;
    ioctl(devfd, BLKGETSIZE64, &sz);
    printf("Disk size is %"PRId64" MB\n", sz/1024/1024);
    return sz/4;
}

//check if cache is disabled, the result should be around 5ms for read and write
void checkCache(int devfd){ 
    struct timeval t1,t2;
    void *checkingbuff;
    float iotime = 0;
    int numiter = 100;
    int CHECK_SIZE = 4096;
    int64_t BLOCK_RANGE = DISK_SIZE/ CHECK_SIZE;

    printf("Checking cache by doing reads...\n");
    
    if (posix_memalign(&checkingbuff,MEM_ALIGN,CHECK_SIZE)){
        fprintf(stderr,"memory allocation for cache checking failed\n");
        exit(1);
    }
    
    int i;
    
    // check read first
    for(i = 0; i < numiter; i++){
        gettimeofday(&t1,NULL); //reset the start time to before start doing the job
        if(pread(fd, checkingbuff, CHECK_SIZE, (off_t)(rand() % BLOCK_RANGE) * CHECK_SIZE) < 0){
            fprintf(stderr,"Read checking failed!\n");
            exit(1);
        }
        gettimeofday(&t2,NULL);
        iotime += (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
    }
    printf("Average 4KB read time: %.3fms\n", iotime / numiter);
    
    //reset io time
    iotime = 0.0;
    
    printf("Checking cache by doing writes...\n");
    
    // check write after that
    for(i = 0; i < numiter; i++){
        gettimeofday(&t1,NULL); //reset the start time to before start doing the job
        if(pwrite(fd, checkingbuff, CHECK_SIZE, (off_t)(rand() % BLOCK_RANGE) * CHECK_SIZE) < 0){
            fprintf(stderr,"Write checking failed!\n");
            exit(1);
        }
        gettimeofday(&t2,NULL);
        iotime += (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
    }
    printf("Average 4KB write time: %.3fms\n", iotime / numiter);
    
    
    free(checkingbuff);
    printf("==============================\n");
}

void cleanCache(){
    printf("Flush the on-disk cache...\n");
    void* cleanbuff;
    if (posix_memalign(&cleanbuff,MEM_ALIGN,500 * 1024 * 1024)){
        fprintf(stderr,"memory allocation for cleaning failed\n");
        exit(1);
    }
    int i;
    for(i = 0; i < 2; i++){
        if(pread(fd, cleanbuff, 500 * 1024 * 1024, 966367641600) < 0){
            fprintf(stderr,"Cannot read 500MB for trace cleaning!\n");
            exit(1);
        }
    }
    free(cleanbuff);
}

void prepareMetrics(){
    if(printlatency == 1 && totalio > maxio){
        fprintf(stderr, "Too many IO in the trace file!\n");
        exit(1);
    }
    if(printlatency == 1){
        metrics = fopen("replay_metrics.txt", "w+");
        
        if(!metrics){
            fprintf(stderr,"Error creating metrics file!\n");
            exit(1);
        }
    }
}

int readTrace(char ***req, char *tracefile){
    //first, read the number of lines
    FILE *trace = fopen(tracefile,"r");
    if (trace == NULL) {
        printf("Cannot open trace file: %s!\n", tracefile);
    }
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
        exit(1);
    }
    
    char line[100]; //assume it will not exceed 100 chars
    int i = 0;
    while(fgets(line, sizeof(line), trace) != NULL){
        line[strlen(line) - 1] = '\0';
        if(((*req)[i] = malloc((strlen(line) + 1) * sizeof(char))) == NULL){
            fprintf(stderr,"Error in memory allocation\n");
            exit(1);
        } //error here
        strcpy((*req)[i],line);
        i++;
    }
    fclose(trace);
    
    return numlines;
}

void arrangeIO(char **requestarray){
    blkno = malloc(totalio * sizeof(int64_t));
    reqsize = malloc(totalio * sizeof(int));
    reqflag = malloc(totalio * sizeof(int));
    timestamp = malloc(totalio * sizeof(float));
    
    if(blkno == NULL || reqsize == NULL || reqflag == NULL || timestamp == NULL){
        fprintf(stderr,"Error malloc in arrangeIO!\n");
        exit(1);
    }
    
    int i = 0;
    for(i = 0; i < totalio; i++){
        char *io = malloc((strlen(requestarray[i]) + 1) * sizeof(char));
        if(io == NULL){
            fprintf(stderr,"Error malloc io!\n");
            exit(1);
        }
        strcpy(io,requestarray[i]);
        
        timestamp[i] = atof(strtok(io," ")); //1. request arrival time
        strtok(NULL," "); //2. device number
        blkno[i] = (int64_t)atoll(strtok(NULL, " ")) % DISK_SIZE;
        blkno[i] *= block_size; //3. block number
        blkno[i] %= DISK_SIZE;
        if (blkno[i] < 0 || blkno[i] >= DISK_SIZE)
            printf("Offset %"PRId64" is larger than disk size OR NEGATIVE!\n", blkno[i]);
        reqsize[i] = atoi(strtok(NULL," ")) * block_size; //4. request size
        reqflag[i] = atoi(strtok(NULL," ")); //5. request flags
        free(io);

        //printf("%.2f,%ld,%d,%d\n", timestamp[i], blkno[i], reqsize[i],reqflag[i]);
    }
}

void atomicAdd(int *val, int add){
    assert(pthread_mutex_lock(&lock) == 0);
    (*val) += add;
    assert(pthread_mutex_unlock(&lock) == 0);
}

void *performIO(){
    //double sum = 0;
    //int howmany = 0;
    int curtask;
    int mylatecount = 0;
    int myslackcount = 0;
    struct timeval t1,t2;
    
    useconds_t sleep_time;

    while(jobtracker < totalio){
        //firstly save the task to avoid any possible contention later
        assert(pthread_mutex_lock(&lock) == 0);
        curtask = jobtracker;
        jobtracker++;
        assert(pthread_mutex_unlock(&lock) == 0);
        
        //respect time part
        if(respecttime == 1){
            gettimeofday(&t1,NULL); //get current time
            uint64_t elapsedtime = t1.tv_sec * 1000000 + t1.tv_usec - starttime;
            if(elapsedtime <= timestamp[curtask] * 1000){
                sleep_time = (useconds_t)(timestamp[curtask] * 1000) - elapsedtime;
                if(sleep_time > 100000){
                    myslackcount++;
                }    
                usleep(sleep_time);
            }else{ //I am late
                mylatecount++;
            }
        }
          
        //do the job      
        gettimeofday(&t1,NULL); //reset the start time to before start doing the job
        if (reqsize[curtask] % 4096)
            reqsize[curtask] = reqsize[curtask] / 4096 * 4096;
        if (blkno[curtask] % 4096)
            blkno[curtask] = blkno[curtask] / 4096 * 4096;
        int ret;
        if(reqflag[curtask] == 0){
#if 0
            if(ret =pwrite(fd, buff, reqsize[curtask], blkno[curtask]) < 0){
                fprintf(stderr,"Cannot write size %d to offset %lu! ret=%d\n",(reqsize[curtask] / 512), (blkno[curtask] / 512), ret);
                //exit(1);
            }
#endif
        }else{
            if(pwrite(fd, buff, reqsize[curtask], blkno[curtask]) < 0){
                fprintf(stderr,"Cannot write size %d to offset %lu!\n",(reqsize[curtask] / 512), (blkno[curtask] / 512));
            }
#if 0
            if((ret = pread(fd, buff, reqsize[curtask], blkno[curtask])) < 0){
                //if(syscall(548, fd, buff, reqsize[curtask], blkno[curtask]) < 0){
                fprintf(stderr,"Cannot read size %d to offset %"PRId64", ret=%d, errno=%d!\n",(reqsize[curtask] / 512), (blkno[curtask] / 512), ret, errno);
                //exit(1);
            }
#endif
            }
            gettimeofday(&t2,NULL);
            /* Coperd: I/O latency in us */
            int iotime = (t2.tv_sec - t1.tv_sec) * 1e6 + (t2.tv_usec - t1.tv_usec);
            if(printlatency == 1){
                assert(pthread_mutex_lock(&lock) == 0);
                /* 
                 * Coperd: keep consistent with fio latency log format:
                 * 1: timestamp in ms
                 * 2: latency in us
                 * 3: r/w type [0 for w, 1 for r] (this is opposite of fio)
                 * 4: I/O size in bytes
                 * 5: offset in bytes
                 */
                fprintf(metrics,"%.3f,%d,%d,%d,%ld\n",timestamp[curtask], iotime, reqflag[curtask], reqsize[curtask], blkno[curtask]);
                assert(pthread_mutex_unlock(&lock) == 0);
            }
        }
    
    atomicAdd(&latecount, mylatecount);
    atomicAdd(&slackcount, myslackcount);
    
    return NULL;
}

void *printProgress(){
    while(jobtracker <= totalio){
        printf("Progress: %.2f%%\r",(float)jobtracker / totalio * 100);
        fflush(stdout);
        
        if(jobtracker == totalio){
            break;
        }
        
        sleep(1);
    }
    printf("\n");
    
    return NULL;
}

void operateWorkers(){
    struct timeval t1,t2;
    float totaltime;
    
    printf("Start doing requests...\n");
    
    // thread creation
    pthread_t *tid = malloc(numworkers * sizeof(pthread_t));
    if(tid == NULL){
        fprintf(stderr,"Error malloc thread!\n");
        exit(1);
    }
    pthread_t track_thread; //progress
    
    assert(pthread_mutex_init(&lock, NULL) == 0);
    
    int x;
    gettimeofday(&t1,NULL);
    starttime = t1.tv_sec * 1000000 + t1.tv_usec;
    for(x = 0; x < numworkers; x++){
        pthread_create(&tid[x], NULL, performIO, NULL);
    }
    pthread_create(&track_thread, NULL, printProgress, NULL); //progress
    for(x = 0; x < numworkers; x++){
        pthread_join(tid[x], NULL);
    }
    pthread_join(track_thread, NULL); //progress
    gettimeofday(&t2,NULL);
    totaltime = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
    printf("==============================\n");
    printf("Total run time: %.3f ms\n",totaltime);
    if(respecttime == 1){
        printf("Late rate: %.2f%%\n",100 * (float)latecount / totalio);
        printf("Slack rate: %.2f%%\n",100 * (float)slackcount / totalio);
    }
    
    fclose(metrics);
    assert(pthread_mutex_destroy(&lock) == 0);
    
    //run statistics
    system("python statistics.py");
}

int main(int argc, char *argv[]) {
    char device[64];
    char **request;
    
    if (argc != 3){
        printf("Usage: ./replayer /dev/tgt0 tracefile\n");
        exit(1);
    }else{
        printf("%s\n", argv[1]);
        sprintf(device,"%s",argv[1]);
        printf("Disk ==> %s\n", device);
    }

    // start the disk part
    fd = open(device, O_DIRECT | O_SYNC | O_RDWR);
    if(fd < 0) {
        fprintf(stderr,"Cannot open %s\n", device);
        exit(1);
    }

    DISK_SIZE = get_disksz(fd);
    
    if (posix_memalign(&buff,MEM_ALIGN,LARGEST_REQUEST_SIZE * block_size)){
        fprintf(stderr,"memory allocation failed\n");
        exit(1);
    }

    // read the trace before everything else
    totalio = readTrace(&request, argv[2]);
    arrangeIO(request);

    //check cache if needed
    if(check_cache == 1){
        checkCache(fd);
    }
    cleanCache();
    printf("After cleancache\n");
    prepareMetrics();
    printf("After prepareMetrics\n");
    operateWorkers();

    free(buff);
    
    return 0;
}
