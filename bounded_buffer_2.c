//This solution makes use of semaphores.

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

// Global Variables
int num_loops = 1;
int buffer_size = 1;
int num_producers = 1;
int num_consumers = 1;
int prod_ptr = 0;                  // Points to the index at which a newly produced item is inserted
int cons_ptr = 0;                  // Points to the index at which an item is to be consumed
int p_flag = 0;
int loop = 0;

int* buffer;

// Semaphores and lock used in producer/consumer signaling protocol
sem_t empty;
sem_t full;
sem_t mutex;

void ensure(int expression, char *msg) {
  if (expression == 0) {
  	fprintf(stderr, "%s\n", msg);
  	exit(1);
  }
}

void put(int value, int id) {
  ensure(buffer[prod_ptr] == -1, "ERROR: tried to fill a non-empty buffer");
  buffer[prod_ptr] = value;
  prod_ptr = (prod_ptr + 1) % buffer_size;  // Modding ptr by buffer size alleviates the need to bound the ptr to the size of the buffer
  if (value != -2)
    printf("%d %s %d\n", id, "Produced:", value);
  loop++;
}

int get(int id) {
  int tmp = buffer[cons_ptr];                                 // Store the value to be consumed at the idx pointed to by the cons ptr in tmp
  ensure(tmp != -1, "ERROR: tried to get an empty buffer");   // Make sure that the value is not -1 (signifies empty entry in buffer)
  buffer[cons_ptr] = -1;                                      // Render the value at the idx pointed to by cons ptr -1, value signifying empty
  cons_ptr = (cons_ptr + 1) % buffer_size;                    // Advance the cons ptr
  if (tmp != -2)
    printf("%d %s %d\n", id, "Consumed:",tmp);

  return tmp;                                                 // Return tmp
}

void *producer(void *arg) {
  int id = (int) arg;

  int i;
  while (loop < num_loops) {         //p0: Run for the specified number of loops
    sem_wait(&empty);
    sem_wait(&mutex);
    if (loop < num_loops)
      put(loop, id);
    sem_post(&mutex);
    sem_post(&full);
  }

  return NULL;
}

void *consumer(void *arg) {
  int id = (int) arg;

  int tmp = 0;
  int i;
  while (tmp != -2) {                       //c0: Run while producers are still producing (if they're not, there will be nothing to consume.)
    sem_wait(&full);
    sem_wait(&mutex);
    tmp = get(id);
    sem_post(&mutex);
    sem_post(&empty);
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

  // Initialize buffer
  buffer = (int *) malloc(buffer_size * sizeof(int));
  int j;
  for (j = 0; j < buffer_size; j++) {
    buffer[j] = -1;     // The value of -1 will signify an empty slot in the buffer
  }

  // Initialize Semaphores
  sem_init(&empty, 0, buffer_size);  // all entries are empty
  sem_init(&full, 0, 0);             // None of entries are full
  sem_init(&mutex, 0, 1);            // Binary semaphore

  pthread_t pid[num_producers], cid[num_consumers];
  int thread_id = 0;
  int g;
  for (g = 0; g < num_producers; g++) {
  	pthread_create(&pid[g], NULL, producer, (void *) (long long) thread_id);
  	thread_id++;
  }
  int i;
  for (i = 0; i < num_consumers; i++) {
  	pthread_create(&cid[i], NULL, consumer, (void *) (long long) thread_id);
  	thread_id++;
  }
  int k;
  for (k = 0; k < num_producers; k++) {
  	pthread_join(pid[k], NULL);
    p_flag = 1;
    printf("%s %d %s\n", "Producer thread", (int) pid[k],"joined.");
  }

  /*
  In order to prevent consumers from continuously running when there is
  no further data produced, an end of data variable (-2) is appended to the
  queue for however many consumers exist.
  */
  int s;
  for (s = 0; s < num_consumers; s++) {
    sem_wait(&empty);
    sem_wait(&mutex);
    put(-2, -2);
    sem_post(&mutex);
    sem_post(&full);
  }

  int t;
  for (t = 0; t < num_consumers; t++) {
  	pthread_join(cid[t], NULL);
    printf("%s %d %s\n", "Consumer thread", (int) cid[t],"joined.");
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
