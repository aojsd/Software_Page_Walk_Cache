/* CS 519, FALL 2019: HW-1 
 * IPC using shared memory to perform matrix multiplication.
 * Feel free to extend or change any code or functions below.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/syscall.h>


//Add all your global variables and definitions here.
#define MATRIX_SIZE 1000

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

void print_stats();
int** makeMat(int rows, int cols);
void populateMat(int** mat, int rows, int cols);
int** transpose(int** mat1, int rows, int cols);

struct timespec n1, n2;
uint64_t total_time;
int cache_en;
int matrixSize;
int procs;
int **matA, **matB, **solutionMat;

/////////////////////////////////////////////////////////
// Provides elapsed Time between t1 and t2 in nanoseconds
/////////////////////////////////////////////////////////
uint64_t gettimediff(struct timespec *begin, struct timespec *end)
{
	return (end->tv_sec * 1000000000 + end->tv_nsec) -
		(begin->tv_sec * 1000000000 + begin->tv_nsec);
}


/* Stats function that prints the time taken and other statistics that you wish
 * to provide.
 */
void print_stats() {
	uint64_t total_time = gettimediff(&n1, &n2);
	uint64_t pwalk_time = syscall(PAGE_WALK_T);
    uint64_t fwalk_time = syscall(PAGE_FAULT_T);
    uint64_t pmd_hits   = 0;
    uint64_t pmd_misses = 0;
    uint64_t pud_hits   = 0;
    uint64_t pud_misses = 0;
    uint64_t p4d_hits   = 0;
    uint64_t p4d_misses = 0;

    double pwalk_frac = (double) pwalk_time / total_time;
    double fwalk_frac = (double) fwalk_time / total_time;

	for(int i = 0; i < matrixSize; i++){
		pmd_hits += syscall(PMD_HITS, matA[i]);
		pmd_hits += syscall(PMD_HITS, matB[i]);
		pmd_hits += syscall(PMD_HITS, solutionMat[i]);
		pmd_misses += syscall(PMD_MISSES, matA[i]);
		pmd_misses += syscall(PMD_MISSES, matB[i]);
		pmd_misses += syscall(PMD_MISSES, solutionMat[i]);

		pud_hits += syscall(PUD_HITS, matA[i]);
		pud_hits += syscall(PUD_HITS, matB[i]);
		pud_hits += syscall(PUD_HITS, solutionMat[i]);
		pud_misses += syscall(PUD_MISSES, matA[i]);
		pud_misses += syscall(PUD_MISSES, matB[i]);
		pud_misses += syscall(PUD_MISSES, solutionMat[i]);

		p4d_hits += syscall(P4D_HITS, matA[i]);
		p4d_hits += syscall(P4D_HITS, matB[i]);
		p4d_hits += syscall(P4D_HITS, solutionMat[i]);
		p4d_misses += syscall(P4D_MISSES, matA[i]);
		p4d_misses += syscall(P4D_MISSES, matB[i]);
		p4d_misses += syscall(P4D_MISSES, solutionMat[i]);				
	}
	pmd_hits += syscall(PMD_HITS, matA);
	pmd_hits += syscall(PMD_HITS, matB);
	pmd_hits += syscall(PMD_HITS, solutionMat);
	pmd_misses += syscall(PMD_MISSES, matA);
	pmd_misses += syscall(PMD_MISSES, matB);
	pmd_misses += syscall(PMD_MISSES, solutionMat);

	pud_hits += syscall(PUD_HITS, matA);
	pud_hits += syscall(PUD_HITS, matB);
	pud_hits += syscall(PUD_HITS, solutionMat);
	pud_misses += syscall(PUD_MISSES, matA);
	pud_misses += syscall(PUD_MISSES, matB);
	pud_misses += syscall(PUD_MISSES, solutionMat);

	p4d_hits += syscall(P4D_HITS, matA);
	p4d_hits += syscall(P4D_HITS, matB);
	p4d_hits += syscall(P4D_HITS, solutionMat);
	p4d_misses += syscall(P4D_MISSES, matA);
	p4d_misses += syscall(P4D_MISSES, matB);
	p4d_misses += syscall(P4D_MISSES, solutionMat);		

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
}

int** makeMat(int rows, int cols){
	int i;
    int** arr = (int**) syscall(MMAP_TIMER, NULL, (rows) * sizeof(int*),
                        PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if(cache_en){
		syscall(VMA_EN_CACHE, arr);
	}
	for(i = 0; i < rows; i++){
        arr[i] = (int*) syscall(MMAP_TIMER, NULL, (cols) * sizeof(int),
                        PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		if(cache_en){
			syscall(VMA_EN_CACHE, arr[i]);
		}						
	}

	return arr;

}

void populateMat(int** mat, int rows, int cols){
	int i,j;
	for (i = 0; i < rows; i++) { 
        for (j = 0; j < cols; j++) { 
            mat[i][j] = rand() % 100; 
        } 
    }
}

int** transpose(int** mat1, int rows, int cols){

	int i,j;
	int** mat2 = makeMat(cols,rows);
	for(i=0;i<rows;i++)
		for(j=0;j<cols;j++)
			mat2[j][i] = mat1[i][j];
	
	for(i=0;i<rows;i++){
		munmap(mat1[i], cols * sizeof(int));
	}
		
	return mat2;
}

int main(int argc, char const *argv[])
{
    syscall(MM_PARAM_RESET);

    if(argc != 3){
        printf("Invalid Arguments\n");
        printf("Arg 1 - Matrix Size\n");
        printf("Arg 2 - Page Walk Cache Enable:\n"
                "\t 0 for disable\n"
                "\t 1 for enable\n");
        return -1;
    }
	matrixSize = atoi(argv[1]);
	cache_en = atoi(argv[2]);
	
	if (matrixSize > MATRIX_SIZE){
		printf("The provided value was more than the testing limit. Size set to %d\n", MATRIX_SIZE);
		matrixSize = MATRIX_SIZE;
	}
	else if(matrixSize < 1){
		printf("The provided value was less than 1. Size set to 1\n");
		matrixSize = 1;
	}
	

	srand(time(0));
	matA = makeMat(matrixSize, matrixSize);
	populateMat(matA, matrixSize, matrixSize);

	matB = makeMat(matrixSize, matrixSize);
	populateMat(matB, matrixSize, matrixSize);
	int i,j;

	solutionMat = makeMat(matrixSize, matrixSize);

    clock_gettime(CLOCK_REALTIME, &n1);
  
	for(int x = 0; x < matrixSize; x++){
		for(int y = 0; y < matrixSize; y++){
			int val = 0;
			for(int z = 0; z < matrixSize; z++){
				val += matA[x][z] * matB[z][y];
			}
			solutionMat[x][y] = val;
		}
	}	

    clock_gettime(CLOCK_REALTIME, &n2);
	
	print_stats(); 

   return 0;
}


