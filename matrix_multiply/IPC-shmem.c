/* CS 519, FALL 2019: HW-1 
 * IPC using shared memory to perform matrix multiplication.
 * Feel free to extend or change any code or functions below.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>


//Add all your global variables and definitions here.
#define MATRIX_SIZE 1000
#define BUFFER_SIZE 32
#define SHMEM_KEY 1766

double getdetlatimeofday(struct timeval *begin, struct timeval *end);
void print_stats();
int** makeMat(int rows, int cols);
void populateMat(int** mat, int rows, int cols);
int** transpose(int** mat1, int rows, int cols);


struct timeval tvalBefore, tvalAfter;



/* Time function that calculates time between start and end */
double getdetlatimeofday(struct timeval *begin, struct timeval *end)
{
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}


/* Stats function that prints the time taken and other statistics that you wish
 * to provide.
 */
void print_stats() {
	uint64_t total_time = getdetlatimeofday(&tvalBefore,&tvalAfter);
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


}

int** makeMat(int rows, int cols){

	int i;
	int** arr = (int**)malloc(rows*sizeof(int*));
	for(i=0;i<rows;i++)
		arr[i] =(int*) malloc(cols * sizeof(int));
	
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
		free(mat1[i]);
	}
		
	return mat2;
}

void childFunction(int** matA,int** matB, int i, int numProcs, int matrixSize){
	
  int shmid = shmget(SHMEM_KEY,sizeof(int[matrixSize][matrixSize]),0666|IPC_CREAT);
  int (*sharedMatrix)[matrixSize]; 
  sharedMatrix = shmat(shmid,(void*)0,0); 

	int x=0 ,y=0,z=0;
	
	for(x = i; x < matrixSize; x += numProcs){
		for(y=0;y<matrixSize;y++){
			int writeVal = 0;
			for(z = 0; z< matrixSize; z++){
				writeVal += matA[x][z] * matB[y][z];
			}	
      //printf("Value in process %d before write: arr[%d][%d] = %d \n",getpid(),x,y,writeVal);
			sharedMatrix[x][y] = writeVal;
			//printf("Value from process %d at shmem: arr[%d][%d] = %d \n",getpid(),x,y,sharedMatrix[x][y]);
		}
	}

  shmdt(sharedMatrix);
	exit(1);
}


int main(int argc, char const *argv[])
{
	

	int matrixSize = atoi(argv[1]);
	
	if (matrixSize > MATRIX_SIZE){
		printf("The provided value was more than the testing limit. Size set to %d\n", MATRIX_SIZE);
		matrixSize = MATRIX_SIZE;
	}
	else if(matrixSize < 1){
		printf("The provided value was less than 1. Size set to 1\n");
		matrixSize = 1;
	}
	

	srand(time(0));
	int** matA = makeMat(matrixSize,matrixSize);
	populateMat(matA,matrixSize,matrixSize);

	int** matB = makeMat(matrixSize,matrixSize);
	populateMat(matB,matrixSize,matrixSize);
	int i,j;


	printf("Matrix A:\n");
	for(i=0;i<matrixSize;i++){
		for(j=0;j<matrixSize;j++){
			printf("%d ", matA[i][j]);
		}
		printf("\n");
	}    

	printf(" \n Matrix B:\n");
	for(i=0;i<matrixSize;i++){
		for(j=0;j<matrixSize;j++){
			printf("%d ", matB[i][j]);
		}
		printf("\n");
	}

    gettimeofday (&tvalBefore, NULL);


	  matB = transpose(matB,matrixSize,matrixSize);


  	long numProcs = sysconf(_SC_NPROCESSORS_ONLN);
	  //printf("Number of processors available: %d\n",numProcs);
	
	
	numProcs = numProcs>matrixSize ? matrixSize:numProcs;
	pid_t* children = (pid_t*) malloc(sizeof(pid_t) * numProcs);
  
  int shmid = shmget(SHMEM_KEY,sizeof(int[matrixSize][matrixSize]),0666|IPC_CREAT|IPC_PRIVATE); 
  
  if(shmid ==-1){
    printf("Error on getting Shared memes \n");
    exit(-1);
  }
  
  int (*sharedMatrix)[matrixSize] ;
  sharedMatrix = shmat(shmid,NULL,0);
  
	for(i=0;i<numProcs;i++){
		pid_t child = fork();
		if(child == 0){
			childFunction(matA, matB, i, numProcs, matrixSize);
		}
		else if ( child < 0){
           		 printf("Could not create child\n");
       	 	} else {
            		children[i] = child;
  
            		//printf("I have created a child and its pid %d\n", child);
        	}
	}
 
  
  for(i = 0; i < numProcs; i++){
        int status;
        waitpid(children[i], &status, 0);
        //printf("Process %d exited with return code %d\n", children[i], WEXITSTATUS(status));
  }

  
	int x=0 ,y=0;
	int** solutionMat = makeMat(matrixSize,matrixSize);

  //by this point, all the threads have done their thing and now we must
  // read from shared memory region.
  for(x=0;x<matrixSize;x++){
    for(y=0;y<matrixSize;y++){
      solutionMat[x][y] = sharedMatrix[x][y];
    }
  }	

  shmdt(sharedMatrix);
  shmctl(shmid,IPC_RMID,NULL); 


	gettimeofday (&tvalAfter, NULL);

	
	printf("\n MM Solution:\n");
	for(i=0;i<matrixSize;i++){
		for(j=0;j<matrixSize;j++){
			printf("%d ", solutionMat[i][j]);
		}
		printf("\n");
	}    

	print_stats(); 

  for(i=0;i<matrixSize;i++){
		free(matA[i]);
		free(matB[i]);
		free(solutionMat[i]);
	}
	free(matA);
	free(matB);
	free(solutionMat);
    
   return 0;
}


