/*
 * OS HW2 System Call Benchmark
 * Compiled with the --static flag for execution in a lightweight VM
 * Measures the overhead for a page fault
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#define GB                  (size_t) (1024*1024*1024)
#define PAGE_SIZE           4096
#define NUM_ACCESSES        (GB/PAGE_SIZE - 2)
#define NUM_ACCESSES_RAND   10000

#define MM_MULTI_ALLOC  336
#define MM_PARAM_RESET  337
#define VMA_EN_CACHE    338
#define MMAP_TIMER      339
#define PAGE_WALK_T     340
#define PAGE_FAULT_T    341
#define PMD_HITS        342
#define PMD_MISSES      343
#define PUD_HITS        344
#define PUD_MISSES      345
#define P4D_HITS        346
#define P4D_MISSES      347

int num_threads, ret, rand_param, cache_en;
pthread_t threads[16];

uint64_t total_times[16] = {0};
uint64_t pwalk_times[16] = {0};
uint64_t fault_times[16] = {0};
uint64_t pmd_hits_t[16] = {0};
uint64_t pud_hits_t[16] = {0};
uint64_t p4d_hits_t[16] = {0};
uint64_t pmd_misses_t[16] = {0};
uint64_t pud_misses_t[16] = {0};
uint64_t p4d_misses_t[16] = {0};

/////////////////////////////////////////////////////////
// Provides elapsed Time between t1 and t2 in nanoseconds
/////////////////////////////////////////////////////////
uint64_t gettimediff(struct timespec *begin, struct timespec *end)
{
	return (end->tv_sec * 1000000000 + end->tv_nsec) -
		(begin->tv_sec * 1000000000 + begin->tv_nsec);
}

void* do_accesses(void* _tid){
    struct timespec n1, n2;
    uint64_t total_time;
    char* array;
    unsigned long accesses[NUM_ACCESSES];
    long tid = (long) _tid;

    // Allocate a 1GB region of page-aligned memory using 4KB pages
    array = (char*) syscall(MMAP_TIMER, NULL, GB, PROT_READ | PROT_WRITE,
                    MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if(array == MAP_FAILED){
        perror("mmap failed");
        pthread_exit(NULL);
    }

    if(cache_en == 1){
        syscall(VMA_EN_CACHE, array);
    }

    // Moved code down here to differentiate random and semi random
    if(rand_param == 2){
        for(int i = 0; i < NUM_ACCESSES_RAND; i+=4){
            accesses[i] = rand() % (GB);
            int j = i;
            unsigned long semiRandAccesses = accesses[i];
            for(j = i; (j < i+4) && (j < NUM_ACCESSES_RAND); j++){
                semiRandAccesses += PAGE_SIZE;
                semiRandAccesses = semiRandAccesses % (GB);
                accesses[j] =  semiRandAccesses;
            }
        }   
    }
    else{
        for(int i = 0; i < NUM_ACCESSES_RAND; i++){
            accesses[i] = rand() % (GB);
        }
    }

    // Start timer
    clock_gettime(CLOCK_REALTIME, &n1);

    // Select random or sequential accesses
    unsigned long sum = 0;
    if(rand_param == 0){
        // Access the array sequentially
        for(int i = 0; i < GB; i += 4*PAGE_SIZE){
            array[i] = 1;
        }
    }
    else if(rand_param == 1 || rand_param == 2){
        for(int i = 0; i < NUM_ACCESSES_RAND; i++){
            int access = accesses[i];
            sum += array[access];
        }
    }

    // End timer
    clock_gettime(CLOCK_REALTIME, &n2);

    // Calculate total time
    total_times[tid] = gettimediff(&n1, &n2);    
    pwalk_times[tid] = syscall(PAGE_WALK_T);
    fault_times[tid] = syscall(PAGE_FAULT_T);
    pmd_hits_t[tid]  = syscall(PMD_HITS, array);
    pud_hits_t[tid]  = syscall(PUD_HITS, array);
    p4d_hits_t[tid]  = syscall(P4D_HITS, array);
    pmd_misses_t[tid] = syscall(PMD_MISSES, array);
    pud_misses_t[tid] = syscall(PUD_MISSES, array);
    p4d_misses_t[tid] = syscall(P4D_MISSES, array);

    munmap(array, GB); 

    pthread_exit((void*)sum);
}

/////////////////////////////////////////////////////////
// Allocates a 2GB, page-aligned region of memory using mmap()
// Sequentially accesses the first byte of each page, causing
// a page fault on each access, and times the process to measure
// the cost of a page fault.
/////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    srand(time(0));
    
    syscall(MM_PARAM_RESET);

    // Check command line arguments
    if(argc != 4){
        printf("Invalid Arguments\n");
        printf("Arg 1 - Number of Threads\n");
        printf("Arg 2 - Type of Access:\n"
                "\t 0 for sequential\n"
                "\t 1 for random\n"
                "\t 2 for semi-random\n");
        printf("Arg 3 - Page Walk Cache Enable:\n"
                "\t 0 for disable\n"
                "\t 1 for enable\n");
        return -1;
    }

    // Read args
    sscanf(argv[1], "%d", &num_threads);
    sscanf(argv[2], "%d", &rand_param);
    sscanf(argv[3], "%d", &cache_en);

    // Check args
    if(num_threads <= 0){
        printf("Invalid number of threads given, must have at least one thread "
                "and less than 16\n");
        return -1;    
    }
    if(cache_en != 0 && cache_en != 1){
        printf("Invalid Cache Argument\n");
        printf("Arg 3 - Page Walk Cache Enable:\n"
                "\t 0 for disable\n"
                "\t 1 for enable\n"); 
        return -1;       
    }
    if(rand_param < 0 || rand_param > 2){
        printf("Invalid Access Argument\n");
        printf("Arg 2 - Type of Access:\n"
                "\t 0 for sequential\n"
                "\t 1 for random\n"
                "\t 2 for semi-random\n");
        return -1;       
    }

    // Start threads
    for(long i = 0; i < num_threads; i++){
        pthread_create(&threads[i], NULL, do_accesses, (void*) i);
    }

    // Join threads
    for(int i = 0; i < num_threads; i++){
        pthread_join(threads[i], NULL);
    }

    // Get totals
    uint64_t total_time = 0, pwalk_time = 0;
    uint64_t pmd_hits = 0, pmd_misses = 0;
    uint64_t pud_hits = 0, pud_misses = 0;
    uint64_t p4d_hits = 0, p4d_misses = 0;

    for(int i = 0; i < num_threads; i++){
        total_time += total_times[i];
        pwalk_time += pwalk_times[i];
        pmd_hits += pmd_hits_t[i];
        pud_hits += pud_hits_t[i];
        p4d_hits += p4d_hits_t[i];
        pmd_misses += pmd_misses_t[i];
        pud_misses += pud_misses_t[i];
        p4d_misses += p4d_misses_t[i];
    }

    printf( "Average Execution time: %lu ns\n"
            "Average Page Walk time: %lu ns\n"
            "Average PMD Hits: %lu\n"
            "Average PMD Misses: %lu\n"
            "Average PUD Hits: %lu\n"
            "Average PUD Misses: %lu\n"
            "Average P4D Hits: %lu\n"
            "Average P4D Misses: %lu\n",
            total_time / num_threads, pwalk_time / num_threads,
            pmd_hits / num_threads, pmd_misses / num_threads,
            pud_hits / num_threads, pud_misses / num_threads,
            p4d_hits / num_threads, p4d_misses / num_threads);

    return 0;
}