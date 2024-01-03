#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

long long int start_range, end_range;
int num_threads;
double global_sqrt_sum = 0.0;
pthread_mutex_t sum_mutex = PTHREAD_MUTEX_INITIALIZER;

void *calculateSquareRoot1(void *arg) {
    int thread_id = *(int *)arg;
    long long int thread_start = start_range + (end_range - start_range + 1) * thread_id / num_threads;
    long long int thread_end = start_range + (end_range - start_range + 1) * (thread_id + 1) / num_threads - 1;

    for (long long int x = thread_start; x <= thread_end; ++x) {
        global_sqrt_sum += sqrt(x);
    }
   

    pthread_exit(NULL);
}

void* calculateSquareRootSum2(void* arg) {
    int thread_id = *(int *)arg;
    long long int thread_start = start_range + (end_range - start_range + 1) * thread_id / num_threads;
    long long int thread_end = start_range + (end_range - start_range + 1) * (thread_id + 1) / num_threads - 1;

    for (long long int x = thread_start; x <= thread_end; x++) {
        pthread_mutex_lock(&sum_mutex);
        global_sqrt_sum += sqrt(x);
        pthread_mutex_unlock(&sum_mutex);
    }

    pthread_exit(NULL);
}



 void *calculateSquareRoot3(void *arg) {
    int thread_id = *(int *)arg;
    long long int thread_start = start_range + (end_range - start_range + 1) * thread_id / num_threads;
    long long int thread_end = start_range + (end_range - start_range + 1) * (thread_id + 1) / num_threads - 1;

    double local_sqrt_sum = 0.0;
    for (long long int x = thread_start; x <= thread_end; ++x) {
        local_sqrt_sum += sqrt(x);
    }

    pthread_mutex_lock(&sum_mutex);
    global_sqrt_sum += local_sqrt_sum;
    pthread_mutex_unlock(&sum_mutex);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <start> <end> <num_threads> <method>\n", argv[0]);
        return 1;
    }

    start_range = atoll(argv[1]);
    end_range = atoll(argv[2]);
    num_threads = atoi(argv[3]);
    int method = atoi(argv[4]);

    pthread_t threads[num_threads];
    int thread_ids[num_threads];
   switch (method)
   {
   case 1:
    for (int i = 0; i < num_threads; ++i) {
        thread_ids[i] = i;
       int createres  = pthread_create(&threads[i], NULL, calculateSquareRoot1, (void *)&thread_ids[i]);
      printf("%d",createres);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }
    break;
   case 2:

    for (int i = 0; i < num_threads; ++i) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, calculateSquareRootSum2, (void *)&thread_ids[i]);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }
   break;
   case 3:
      for (int i = 0; i < num_threads; ++i) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, calculateSquareRoot3, (void *)&thread_ids[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
   break;
   }
    

    printf("Toplam karekök: %.5e\n", global_sqrt_sum);
    
    return 0;
}
