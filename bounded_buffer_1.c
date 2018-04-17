//This solution makes use of pthread mutex and pthread condition variables.

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

// Global Variables
int num_loops = 1;
int buffer_size = 1;
int num_producers = 1;
int num_consumers = 1;
int num_entries = 0;               // Number of entries in buffer
int prod_ptr = 0;                  // Points to the index at which a newly produced item is inserted
int cons_ptr = 0;                  // Points to the index at which an item is to be consumed
int p_flag = 0;
int loop = 0;

int* buffer;

// Condition vars and lock used in producer/consumer signaling protocol
pthread_cond_t empty  = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill   = PTHREAD_COND_INITIALIZER;
pthread_mutex_t m     = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t l     = PTHREAD_MUTEX_INITIALIZER;

// Used for appending the end of data variable
pthread_cond_t *fill_cv = &fill;
pthread_cond_t *empty_cv = &empty;

void ensure(int expression, char *msg) {
  if (expression == 0) {
  	fprintf(stderr, "%s\n", msg);
  	exit(1);
  }
}

void do_fill(int value, int id) {
  ensure(buffer[prod_ptr] == -1, "ERROR: tried to fill a non-empty buffer");
  buffer[prod_ptr] = value;
  prod_ptr = (prod_ptr + 1) % buffer_size;  // Modding ptr by buffer size alleviates the need to bound the ptr to the size of the buffer
  if (value != -2)
    printf("%d %s %d\n", id, "Produced:", value);
  num_entries++;
}

int do_get(int id) {
  int tmp = buffer[cons_ptr];                                 // Store the value to be consumed at the idx pointed to by the cons ptr in tmp
  ensure(tmp != -1, "ERROR: tried to get an empty buffer");   // Make sure that the value is not -1 (signifies empty entry in buffer)
  buffer[cons_ptr] = -1;                                      // Render the value at the idx pointed to by cons ptr -1, value signifying empty
  cons_ptr = (cons_ptr + 1) % buffer_size;                    // Advance the cons ptr
  num_entries--;                                              // Decrement the number of num_entries
  if (tmp != -2)
    printf("%d %s %d\n", id, "Consumed:",tmp);

  return tmp;                                                 // Return tmp
}

void *producer(void *arg) {
  int id = (int) arg;

  while (1) {                               //p0: Run forever.
    pthread_mutex_lock(&m);                 //p1: Obtain lock before entering critical section (only the thread with the lock will be in cs).
    if (loop < num_loops) {                 //p2: Check if we have reached the number of items we want to produce.
      loop ++;                              //p3: If not, then preemptively increment the loop (in case the current thread has to wait at p5 and give up the lock).
      while (num_entries == buffer_size) {  //p4: Check if # of entries is equal to buff_size (buff is full), if so, then wait for empty.
    	    pthread_cond_wait(&empty, &m);    //p5: Wait/block until the buffer is empty.
    	}
      int r = rand();                       //p6: Generate item to place into buffer.
      do_fill(r, id);                       //p7: Once buffer is empty, call do_fill to enter value into buffer
      pthread_cond_signal(&fill);           //p8: Once the buffer is filled, signal that it has been filled.
    	pthread_mutex_unlock(&m);             //p9: Release the lock
    } else {                                //p10: If we have reached the number of items we want to produce, we need to break out.
      pthread_mutex_unlock(&m);             //p11: Release the lock acquired at p1 to prevent terminating with lock.
      return NULL;                          //p12: Return and join with main (terminate thread).
    }
  }

  return NULL;
}

void *consumer(void *arg) {
  int id = (int) arg;

  int tmp = 0;
  int i;
  while (tmp != -2) {                       //c0: Run while producers are still producing (if they're not, there will be nothing to consume.)
    pthread_mutex_lock(&m);                 //c1: Obtain lock before entering cs.
  	while (num_entries == 0) {              //c2: Check if empty, if so, then wait
	    pthread_cond_wait(&fill, &m);         //c3: Wait for the buffer to be filled
    }
  	tmp = do_get(id);                         //c4: Once buffer is filled, consume
  	pthread_cond_signal(&empty);            //c5: Signal that buffer contents have been consumed and buff is now empty
  	pthread_mutex_unlock(&m);               //c6: Release the lock
  }

  return NULL;
}

int main(int argc, char *argv[]) {
  // Parse the arguments
  if(argc != 5) {
    printf("%s\n", "Argument structure: <INT(Number of producers)> <INT(Number of consumers)> <INT(Buffer size)> <INT(Number of loops)>");
  }

  num_producers = atoi(argv[1]);
  num_consumers = atoi(argv[2]);
  buffer_size = atoi(argv[3]);
  num_loops = atoi(argv[4]);

  srand(time(NULL));

  // Initialize buffer
  buffer = (int *) malloc(buffer_size * sizeof(int));
  int j;
  for (j = 0; j < buffer_size; j++) {
    buffer[j] = -1;     // The value of -1 will signify an empty slot in the buffer
  }

  pthread_t pid[num_producers], cid[num_consumers];
  int thread_id = 0;
  int i;
  for (i = 0; i < num_producers; i++) {
  	pthread_create(&pid[i], NULL, producer, (void *) (long long) thread_id);
  	thread_id++;
  }
  for (i = 0; i < num_consumers; i++) {
  	pthread_create(&cid[i], NULL, consumer, (void *) (long long) thread_id);
  	thread_id++;
  }

  for (i = 0; i < num_producers; i++) {
  	pthread_join(pid[i], NULL);
    p_flag = 1;
    printf("%s %d %s\n", "Producer thread", (int) pid[i],"joined.");
  }

  /*
  In order to prevent consumers from continuously running when there is
  no further data produced, an end of data variable (-2) is appended to the
  queue for however many consumers exist.
  */
  for (i = 0; i < num_consumers; i++) {
    pthread_mutex_lock(&m);
  	while (num_entries == buffer_size)
  	    pthread_cond_wait(empty_cv, &m);
  	do_fill(-2, -2);
  	pthread_cond_signal(fill_cv);
  	pthread_mutex_unlock(&m);
  }

  for (i = 0; i < num_consumers; i++) {
  	pthread_join(cid[i], NULL);
    printf("%s %d %s\n", "Consumer thread", (int) cid[i],"joined.");
  }

  return 0;
}

/*
==============================SOURCES=========================================
Course Textbook, provided code from textbook
https://github.com/asnr/ostep/tree/master/concurrency/30_condition_variables
Code above is based off of the following files from the above link:
main-header.h
main-common.c
main-two-cvs-while.c
https://macboypro.wordpress.com/2009/05/25/producer-consumer-problem-using-cpthreadsbounded-buffer/
==============================================================================
*/
