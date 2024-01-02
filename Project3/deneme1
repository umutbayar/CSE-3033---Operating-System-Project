#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <sys/time.h>

// Global variables
double global_sqrt_sum = 0;
long long int a, b;
int thread_count;
int method;
pthread_mutex_t mutex;

// Function to calculate the sum of square roots in a range for each thread
void* calculate_sqrt_sum(void* rank) {
  long long int start = *((long long int *)rank);
    long long int end = start + (b - a + 1) / thread_count - 1;
    double local_sqrt_sum = 0;

    

    // Yönteme göre toplam değişkenini güncelle
    switch (method) {
        case 1:
            for (long long int x = start; x <= end; ++x) {
        global_sqrt_sum += sqrt(x);
           }
            break;
        case 2:
            // Method 2: Mutex ile sıralı güncelleme
            pthread_mutex_lock(&mutex);
            for (long long int x = start; x <= end; ++x) {
        global_sqrt_sum += sqrt(x);
              }
            pthread_mutex_unlock(&mutex);
            break;
        case 3:
            
            pthread_mutex_lock(&mutex);
            for (long long int x = start; x <= end; ++x) {
        local_sqrt_sum += sqrt(x);
           }
            global_sqrt_sum += local_sqrt_sum;
            pthread_mutex_unlock(&mutex);
            break;
        default:
            break;
    }

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <start> <end> <thread_count> <method>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parse command line arguments
    a = atoll(argv[1]);
    b = atoll(argv[2]);
    thread_count = atoi(argv[3]);
    method = atoi(argv[4]);

    // Initialize mutex
    pthread_mutex_init(&mutex, NULL);

    // Create threads
    pthread_t* thread_handles = (pthread_t*)malloc(thread_count * sizeof(pthread_t));
    long long int *threadStarts = (long long int *)malloc(thread_count * sizeof(long long int));
    // Measure start time
    struct timeval start, end;
    gettimeofday(&start, NULL);

    for (long thread = 0; thread < thread_count; thread++) {
        threadStarts[thread] = thread * (b - a + 1) / thread_count + a;
        pthread_create(&thread_handles[thread], NULL, calculate_sqrt_sum, (void *)&threadStarts[thread]);
    }

    // Join threads
    for (long thread = 0; thread < thread_count; thread++) {
        pthread_join(thread_handles[thread], NULL);
    }

    // Measure end time
    gettimeofday(&end, NULL);

    // Print the result
    printf("Sum of square roots in the range %lld-%lld: %f\n", a, b, global_sqrt_sum);

    // Print time information
    printf("User time: %ld ms\n", (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000);
    printf("System time: %ld ms\n", (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000);
    printf("Total time: %ld ms\n", (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000);

    // Clean up
    free(thread_handles);
    pthread_mutex_destroy(&mutex);

    return 0;
}
