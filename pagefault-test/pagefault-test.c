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
#define MMU_INV_COUNT   338
#define MMAP_MMU_INV    339
#define PARTIAL_WALK_T  340
#define FULL_WALK_T     341

struct timespec n1, n2;
uint64_t total_time;
char* array;
char accesses[NUM_ACCESSES];

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
    for(int i = 0; i < GB - 2*PAGE_SIZE; i += PAGE_SIZE){
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
    srand(time(NULL));
    for(int i = 0; i < NUM_ACCESSES_RAND; i++){
        accesses[i] = rand() % (GB - PAGE_SIZE);
    }

    syscall(MM_PARAM_RESET);

    // Check command line arguments
    if(argc != 3){
        printf("Invalid Arguments\n");
        return -1;
    }

    // Determine page multi alloc parameter
    int multi_alloc, ret, rand_param;
    sscanf(argv[1], "%d", &multi_alloc);
    sscanf(argv[2], "%d", &rand_param);
    ret = syscall(MM_MULTI_ALLOC, multi_alloc);
    if(ret != multi_alloc){
        printf("multi alloc set failed\n");
        return -1;
    }

    // Allocate a 1MB region of page-aligned memory using 4KB pages
    array = (char*) syscall(MMAP_MMU_INV, NULL, GB - PAGE_SIZE, PROT_READ | PROT_WRITE,
                    MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if(array == MAP_FAILED){
        perror("mmap1 failed");
        return -1;
    }

    // Start timer
    clock_gettime(CLOCK_REALTIME, &n1);

    // Select random or sequential accesses
    if(!rand_param){
        seq_access();
        goto end;
    }

    for(int i = 0; i < NUM_ACCESSES_RAND; i++){
        int access = accesses[i];
        array[i] = 1;
    }

    // End timer
end:
    clock_gettime(CLOCK_REALTIME, &n2);
    munmap(array, GB - PAGE_SIZE);

    // Calculate total time
    uint64_t total_time = gettimediff(&n1, &n2);    
    uint64_t pwalk_time = syscall(PARTIAL_WALK_T);
    uint64_t fwalk_time = syscall(FULL_WALK_T);

    double pwalk_frac = (double) pwalk_time / total_time;
    double fwalk_frac = (double) fwalk_time / total_time;

    printf( "Execution time: %lu ns\n"
            "Partial walk time: %lu ns\n"
            "Full walk time: %lu ns\n"
            "Fraction spent on partial page walks: %lf\n"
            "Fraction spent on faults excluding walks: %lf\n",
            total_time, pwalk_time, fwalk_time, pwalk_frac, fwalk_frac);

    return 0;
}