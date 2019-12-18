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
#include <sys/mman.h>
#include <sys/syscall.h>

#define GB                  (size_t) (1024*1024*1024)
#define PAGE_SIZE           4096
#define NUM_ACCESSES        (GB/PAGE_SIZE - 2)
#define NUM_ACCESSES_RAND   1000

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

struct timespec n1, n2;
uint64_t total_time;
uint64_t j_start, j_end;
char* array;
unsigned long accesses[NUM_ACCESSES];

/////////////////////////////////////////////////////////
// Provides elapsed Time between t1 and t2 in nanoseconds
/////////////////////////////////////////////////////////
uint64_t gettimediff(struct timespec *begin, struct timespec *end)
{
	return (end->tv_sec * 1000000000 + end->tv_nsec) -
		(begin->tv_sec * 1000000000 + begin->tv_nsec);
}

void seq_access(){
    // Access the array sequentially
    for(int i = 0; i < GB; i += 4*PAGE_SIZE){
        array[i] = 1;
    }
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
        printf("Arg 1 - Multi-alloc Parameter\n");
        printf("Arg 2 - Type of Access:\n"
                "\t 0 for sequential\n"
                "\t 1 for random\n"
                "\t 2 for semi-random\n");
        printf("Arg 3 - Page Walk Cache Enable:\n"
                "\t 0 for disable\n"
                "\t 1 for enable\n");
        return -1;
    }

    // Determine page multi alloc parameter
    //Rand_param 0->sequential,1->random,2->semirandom
    int multi_alloc, ret, rand_param, cache_en;
    sscanf(argv[1], "%d", &multi_alloc);
    sscanf(argv[2], "%d", &rand_param);
    sscanf(argv[3], "%d", &cache_en);

    ret = syscall(MM_MULTI_ALLOC, multi_alloc);
    if(ret != multi_alloc){
        printf("multi alloc set failed\n");
        return -1;
    }

    // Allocate a 1MB region of page-aligned memory using 4KB pages
    array = (char*) syscall(MMAP_TIMER, NULL, GB, PROT_READ | PROT_WRITE,
                    MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if(array == MAP_FAILED){
        perror("mmap failed");
        return -1;
    }

    if(cache_en == 1){
        syscall(VMA_EN_CACHE, array);
    }
    else if (cache_en != 0){
        printf("Invalid Cache Argument\n");
        printf("Arg 3 - Page Walk Cache Enable:\n"
                "\t 0 for disable\n"
                "\t 1 for enable\n"); 
        return -1;       
    }

    // Moved code down here to differentiate random and semi random
    if(rand_param == 2){
        for(int i = 0; i < NUM_ACCESSES_RAND; i+=4){
            accesses[i] = rand() % (GB);
            int j = i;
            unsigned long semiRandAccesses = accesses[i];
            for(j = i; (j < i+4) && (j < NUM_ACCESSES_RAND); j++){
                semiRandAccesses += PAGE_SIZE;
                semiRandAccesses = semiRandAccesses % (GB - 2 * PAGE_SIZE);
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
    unsigned int sum = 0;
    if(rand_param == 0){
        seq_access();
    }
    else if(rand_param == 1 || rand_param == 2){
        for(int i = 0; i < NUM_ACCESSES_RAND; i++){
            int access = accesses[i];
            sum += array[access];
        }
    }
    else {
        printf("Invalid Access Argument\n");
        printf("Arg 2 - Type of Access:\n"
                "\t 0 for sequential\n"
                "\t 1 for random\n"
                "\t 2 for semi-random\n");
        return -1;       
    }

    // End timer
    clock_gettime(CLOCK_REALTIME, &n2);

    // Calculate total time
    uint64_t total_time = gettimediff(&n1, &n2);    
    uint64_t pwalk_time = syscall(PAGE_WALK_T);
    uint64_t fwalk_time = syscall(PAGE_FAULT_T);
    uint64_t pmd_hits   = syscall(PMD_HITS, array);
    uint64_t pmd_misses = syscall(PMD_MISSES, array);
    uint64_t pud_hits   = syscall(PUD_HITS, array);
    uint64_t pud_misses = syscall(PUD_MISSES, array);
    uint64_t p4d_hits   = syscall(P4D_HITS, array);
    uint64_t p4d_misses = syscall(P4D_MISSES, array);    

    double pwalk_frac = (double) pwalk_time / total_time;
    double fwalk_frac = (double) fwalk_time / total_time;

    printf( "Execution time: %lu ns\n"
            "Page Walk time: %lu ns\n"
            "PMD Hits: %lu\n"
            "PMD Misses: %lu\n"
            "PUD Hits: %lu\n"
            "PUD Misses: %lu\n"
            "P4D Hits: %lu\n"
            "P4D Misses: %lu\n",
            total_time, pwalk_time,
            pmd_hits, pmd_misses, pud_hits, pud_misses, p4d_hits, p4d_misses);

    munmap(array, GB - PAGE_SIZE);
    return sum;
}