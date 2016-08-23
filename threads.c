/* Evan Woo
   Ruchir Patel
   Evan Satinsky

   The project Description: The group project that demonstrates how multithreading can improve
   performance in many operations by allowing computations to run concurrently.

*/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>
#include <string.h>

/*Global variables to allow efficient calculations.*/
static long sum = 0;
static long actual = 0;
static pthread_mutex_t mutex;
static long max = 0;

/*array struct used to store array and necessary fields*/
struct array {
  int *arr;
  int count;
  int threads_run;
  int seg;
  int num_elements;
};

/*timeval struct used in time calculations*/
struct timeval tv_delta(struct timeval start, struct timeval end) {
  struct timeval delta = end;

  delta.tv_sec -= start.tv_sec;
  delta.tv_usec -= start.tv_usec;
  if (delta.tv_usec < 0) {
    delta.tv_usec += 1000000;
    delta.tv_sec--;
  }

  return delta;
}

static void *print_sum(void *array);
static void *print_max(void *array);
static void *print_sum_end(void *array);
static void *print_max_end(void *array);

/*This function executes the threads on a randomly filled array based on the
  arguments provided. It uses the function command arguments to generate the 
  random numbers. The beginning of the main method tries to call the
  multithreads.*/
int main(int argc, char *argv[]) {

    struct rusage start_ru, end_ru;
    struct timeval start_wall, end_wall;
    struct timeval diff_ru_utime, diff_wall, diff_ru_stime;

    struct array array;

    int num_elem = atoi(argv[1]), i, num_threads = atoi(argv[2]),
      task = atoi(argv[4]);
    unsigned int seed = atoi(argv[3]);
    pthread_t *tids = malloc(sizeof(pthread_t) * num_threads);

    array.num_elements = num_elem;
    array.seg = num_elem/num_threads; /*number of elements run in each thread*/
    array.arr = malloc(sizeof(int) * num_elem);
    array.count = 0;

    /*Setting the seed and filling the array while calculating the actual sum.*/
    srand(seed);
    for (i = 0; i < num_elem; i++) {
      array.arr[i] = rand() % 100;
      printf("Element %d: %d\n", i, array.arr[i]);
      actual += array.arr[i];
    }

    /*Collecting information*/
    getrusage(RUSAGE_SELF, &start_ru);
    gettimeofday(&start_wall, NULL);

    for (i = 0; i < num_threads; i++) {
      /*Last thread processes the rest of the elements left in the array.*/
      if (task == 1) {
        if (i == num_threads - 1) {
          pthread_create(&tids[i], NULL, print_max_end, &array);
        }
        else {
          pthread_create(&tids[i], NULL, print_max, &array);
          array.threads_run++;
        }
      }
      else if (task == 2) {
        if (i == num_threads - 1) {
          pthread_create(&tids[i], NULL, print_sum_end, &array);
        }
        else {
          pthread_create(&tids[i], NULL, print_sum, &array);
          array.threads_run++;
        }
      }
    }
    /*Join threads so all terminate at the end of the loop*/
    for (i = 0; i < num_threads; i++) {
      pthread_join(tids[i], NULL);
    }

    /*Collecting end times*/
    gettimeofday(&end_wall, NULL);
    getrusage(RUSAGE_SELF, &end_ru);

    /*Computing differences between times*/
    diff_ru_utime = tv_delta(start_ru.ru_utime, end_ru.ru_utime);
    diff_ru_stime = tv_delta(start_ru.ru_stime, end_ru.ru_stime);
    diff_wall = tv_delta(start_wall, end_wall);

    /*Print appropriate output if Y provided*/
    if (!strcmp(argv[5], "Y")) {
      if (task == 1) {
        printf("Max: %ld\n", max);
      }
      else if (task == 2) {
        printf("Sum: %ld\n", sum);
        printf("Actual: %ld\n", actual);
      }
    }

    /*Print different times*/
    printf("User time: %ld.%06ld\n", diff_ru_utime.tv_sec,
           diff_ru_utime.tv_usec);
    printf("System time: %ld.%06ld\n", diff_ru_stime.tv_sec,
           diff_ru_stime.tv_usec);
    printf("Wall time: %ld.%06ld\n", diff_wall.tv_sec, diff_wall.tv_usec);

    free(tids);
    free(array.arr);
    return 0;
}

/*Each thread if user wants sum will add a certain number of elements in the
  array to the global sum variable. In order to keep the calculated the sum,
  it should lock the calculated sum before adding to prevent
  data race.*/
static void *print_sum(void *ptr) {
  struct array *nums = (struct array *)ptr;

  while (nums->count < nums->seg * nums->threads_run) {
    pthread_mutex_lock(&mutex);
    sum += nums->arr[nums->count];
    nums->count++;
    pthread_mutex_unlock(&mutex);
  }

  return NULL;
}

/*Function for computing sum of remaining elements. 
  The function calls the lock because it tries to keep
  the right concurrency while printing out the correct sum.*/
static void *print_sum_end(void *ptr) {
  struct array *nums = (struct array *)ptr;

  while (nums->count < nums->num_elements) {
    pthread_mutex_lock(&mutex);
    sum += nums->arr[nums->count];
    nums->count++;
    pthread_mutex_unlock(&mutex);
  }

  return NULL;
}


/*Each thread will process a segment of the array and check and update max.*/
static void *print_max(void *ptr) {
  struct array *nums = (struct array *)ptr;
  while (nums->count < nums->seg * nums->threads_run) {
    if (nums->arr[nums->count] > max) {
      max = nums->arr[nums->count];
    }
    nums->count++;
  }

  return NULL;
}

/*Function for computing max of end segment of array.*/
static void *print_max_end(void *ptr) {
  struct array *nums = (struct array *)ptr;

  while (nums->count < nums->num_elements) {
    if (nums->arr[nums->count] > max) {
      max = nums->arr[nums->count];
    }
    nums->count++;
  }

  return NULL;
}