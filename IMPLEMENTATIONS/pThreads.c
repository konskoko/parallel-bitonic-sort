#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>


struct timeval startwtime, endwtime;
double seq_time;

int currentThreads = 0, maxThreads;

int N;          // data array size
int *a;         // data array to be sorted
pthread_mutex_t tC;

const int ASCENDING  = 1;
const int DESCENDING = 0;


void init(void);
void print(void);
void sort(void);
void test(void);
void exchange(int i, int j);
void compare(int i, int j, int dir);
void bitonicMerge(int lo, int cnt, int dir);

//added functions
void *PrecBitonicSort(void *);
void Psort();
int asc(const void * a, const void * b);
int desc(const void * a, const void * b);

/**arguments structure**/
typedef struct {
	int lo;
	int cnt;
	int dir;
	}parm;


/** the main program **/
int main(int argc, char **argv) {


  if (argc != 3) {
    printf("Usage: %s q p\n  where n=2^q is problem size (power of two)\n  and t=2^p is the number of threads (power of two)\n", 
	   argv[0]);
    exit(1);
  }
  
  
  N = 1<<atoi(argv[1]);
  maxThreads = 1<<atoi(argv[2]);
  a = (int *)malloc(N*sizeof(int));
  
  pthread_mutex_init(&tC, NULL);
  
  //time and test parallel version
  init();
  gettimeofday (&startwtime, NULL);
  Psort();
  gettimeofday (&endwtime, NULL);

  seq_time = (double)((endwtime.tv_usec - startwtime.tv_usec)/1.0e6
		      + endwtime.tv_sec - startwtime.tv_sec);

  printf("Parallel Recursive wall clock time = %f\n", seq_time);
  test();

  pthread_mutex_destroy(&tC);
	
  free(a);

}

/** -------------- SUB-PROCEDURES  ----------------- **/

//compare functions for qsort
int asc(const void * a, const void * b) {
   return ( *(int*)a - *(int*)b );
}

int desc(const void * a, const void * b) {
   return -( *(int*)a - *(int*)b );
}

//Parallel sort function
void Psort() {
    parm arg;
    arg.lo = 0;
    arg.cnt = N;
    arg.dir = ASCENDING;
    PrecBitonicSort((void *)&arg);
  }

/** procedure test() : verify sort results **/
void test() {
  int pass = 1;
  int i;
  for (i = 1; i < N; i++) {
    pass &= (a[i-1] <= a[i]);
  }

  printf(" TEST %s\n",(pass) ? "PASSed" : "FAILed");
}


/** procedure init() : initialize array "a" with data **/
void init() {
  int i;
  for (i = 0; i < N; i++) {
    a[i] = rand() % N; // (N - i);
  }
}

/** procedure  print() : print array elements **/
void print() {
  int i;
  for (i = 0; i < N; i++) {
    printf("%d\n", a[i]);
  }
  printf("\n");
}


/** INLINE procedure exchange() : pair swap **/
inline void exchange(int i, int j) {
  int t;
  t = a[i];
  a[i] = a[j];
  a[j] = t;
}



/** procedure compare()
   The parameter dir indicates the sorting direction, ASCENDING
   or DESCENDING; if (a[i] > a[j]) agrees with the direction,
   then a[i] and a[j] are interchanged.
**/
inline void compare(int i, int j, int dir) {
  if (dir==(a[i]>a[j]))
    exchange(i,j);
}

/** Procedure bitonicMerge()
   It recursively sorts a bitonic sequence in ascending order,
   if dir = ASCENDING, and in descending order otherwise.
   The sequence to be sorted starts at index position lo,
   the parameter cbt is the number of elements to be sorted.
 **/
 
void bitonicMerge(int lo, int cnt, int dir) {
  if (cnt>1) {
    int k=cnt/2;
    int i;
    for (i=lo; i<lo+k; i++)
      compare(i, i+k, dir);
    bitonicMerge(lo, k, dir);
    bitonicMerge(lo+k, k, dir);
  }
}

//parallel version of recBitonicSort
void *PrecBitonicSort(void *arg) {
	
  if (((parm* )arg)->cnt>1) {
    parm p = *(parm*)arg;
    int k=p.cnt/2;
    //check if allowed to create threads
    if(currentThreads <= maxThreads-2) {
    	//add 2 to thread counter (protected by mutex)
    	pthread_mutex_lock(&tC);
    	currentThreads += 2;
    	pthread_mutex_unlock(&tC);
    	
		//declare the 2 threads and their parameters
		pthread_t t1,t2;
		parm p1,p2;
		
		//set up parameters
		p1.lo = p.lo;
		p1.cnt = k;
		p1.dir = ASCENDING;

		p2.lo = p.lo + k;
		p2.cnt = k;
		p2.dir = DESCENDING;
		
		pthread_create(&t1, NULL, PrecBitonicSort,(void*)&p1);
		pthread_create(&t2, NULL, PrecBitonicSort,(void*)&p2);

		pthread_join(t1, NULL);
		pthread_join(t2, NULL);
    }
    //else use serial qsort
    else {
	    qsort(a + p.lo, k, sizeof(int), asc);
    	qsort(a + p.lo + k, k, sizeof(int), desc);
    }
    
	bitonicMerge(p.lo,p.cnt,p.dir);
  }
}
