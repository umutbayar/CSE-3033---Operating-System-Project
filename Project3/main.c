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
    long my_rank = (long)rank;
    long long int local_a = a + my_rank * (b - a + 1) / thread_count;
    long long int local_b = a + (my_rank + 1) * (b - a + 1) / thread_count - 1;

    if ( method ==1){
        double local_sqrt_sum=0;
    for (long long int x = local_a; x <= local_b; x++) {
        local_sqrt_sum += sqrt(x);
    }

        global_sqrt_sum += local_sqrt_sum; // Method 1: No mutex is used
    } else if (method == 2 || method == 3) {

        double local_sqrt_sum=0;

        for (long long int x = local_a; x <= local_b; x++) {
        local_sqrt_sum += sqrt(x);
    }
        // Method 2 and 3: Mutex is used to protect the global sum
        pthread_mutex_lock(&mutex);
        global_sqrt_sum += local_sqrt_sum;
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
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

    // Measure start time
    struct timeval start, end;
    gettimeofday(&start, NULL);

    for (long thread = 0; thread < thread_count; thread++) {
        pthread_create(&thread_handles[thread], NULL, calculate_sqrt_sum, (void*)thread);
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
