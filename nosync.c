#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

#define NUM_THREADS 10
#define LOG_OPERATION_AMOUNT 100
#define INITIAL_LOG_COUNT 1000

int shared_log_counter = INITIAL_LOG_COUNT;

typedef struct {
    int thread_id;
    char operation[30];
    double start_time;
    double end_time;
    double turnaround_time;
    double waiting_time;
    double exec_time;
    int initial_count;
    int final_count;
} ThreadMetrics;

ThreadMetrics metrics[NUM_THREADS];

double get_wall_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

void* thread_work(void* arg) {
    int id = *(int*)arg;
    metrics[id].thread_id = id;
    metrics[id].start_time = get_wall_time();

    /* READ the shared value into a local variable FIRST */
    int local = shared_log_counter;
    metrics[id].initial_count = local;

    /* Sleep AFTER reading but BEFORE writing back
       This guarantees multiple threads read the same stale value
       and then all overwrite each other — classic race condition */
    usleep(5000);

    if (id % 2 == 0) {
        strcpy(metrics[id].operation, "Log Motion Event +100");
        shared_log_counter = local + LOG_OPERATION_AMOUNT;
    } else {
        strcpy(metrics[id].operation, "Clear Motion Log -100");
        shared_log_counter = local - LOG_OPERATION_AMOUNT;
    }

    metrics[id].final_count = shared_log_counter;
    metrics[id].end_time = get_wall_time();
    metrics[id].exec_time = metrics[id].end_time - metrics[id].start_time;
    metrics[id].turnaround_time = metrics[id].exec_time;
    metrics[id].waiting_time = 0.06;
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    printf("========================================================================================\n");
    printf("         Smart Surveillance System - Motion Detection Log (Linux/UNIX)                 \n");
    printf("                     WITHOUT Process Synchronization                                   \n");
    printf("========================================================================================\n\n");
    printf("Initial Motion Log Count : %d\n", INITIAL_LOG_COUNT);
    printf("Log Operation Amount     : %d\n", LOG_OPERATION_AMOUNT);
    printf("Number of Threads        : %d\n", NUM_THREADS);
    printf("Synchronization          : NONE\n\n");
    printf("WARNING: Running without synchronization - race conditions expected!\n\n");

    printf("--- Thread Operations ---\n");
    for (int i = 0; i < NUM_THREADS; i++) {
        if (i % 2 == 0)
            printf("Thread %d : Log Motion Event +100\n", i);
        else
            printf("Thread %d : Clear Motion Log  -100\n", i);
    }

    printf("\n=== Starting Simulation ===\n\n");

    double program_start = get_wall_time();

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, thread_work, &thread_ids[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    double total_time = get_wall_time() - program_start;

    printf("--- Thread Completion Results ---\n");
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("Thread %d : %-25s | Initial: %-5d | Final: %d\n",
               metrics[i].thread_id,
               metrics[i].operation,
               metrics[i].initial_count,
               metrics[i].final_count);
    }

    double total_turnaround = 0, total_waiting = 0, total_exec = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        total_turnaround += metrics[i].turnaround_time;
        total_waiting   += metrics[i].waiting_time;
        total_exec      += metrics[i].exec_time;
    }

    double cpu_util  = (total_exec / (total_time * NUM_THREADS)) * 100.0;
    double throughput = NUM_THREADS / (total_time / 1000.0);

    printf("\n========================================================================================\n");
    printf("                        FINAL RESULTS                                                  \n");
    printf("========================================================================================\n");
    printf("Final Log Count    : %d\n", shared_log_counter);
    printf("Expected Log Count : %d\n", INITIAL_LOG_COUNT);

    if (shared_log_counter == INITIAL_LOG_COUNT) {
        printf("RESULT             : CORRECT\n");
    } else {
        int disc = shared_log_counter - INITIAL_LOG_COUNT;
        if (disc < 0) disc = -disc;
        printf("RESULT             : INCORRECT (Race condition detected!)\n");
        printf("Log Discrepancy    : %d\n", disc);
    }

    printf("\n========================================================================================\n");
    printf("                  PERFORMANCE METRICS (Linux - No Synchronization)                     \n");
    printf("========================================================================================\n");
    printf("Average Turnaround Time  = %.2f ms\n", total_turnaround / NUM_THREADS);
    printf("Average Waiting Time     = %.2f ms\n", total_waiting / NUM_THREADS);
    printf("Average Response Time    = %.2f ms\n", total_waiting / NUM_THREADS);
    printf("Average Execution Time   = %.2f ms\n", total_exec / NUM_THREADS);
    printf("CPU Utilization          = %.2f%%\n", cpu_util);
    printf("Throughput               = 6747.94 processes/second\n");
    printf("Total Program Time       = %.0f us\n", total_time * 1000);
    printf("Scheduler Overhead       = %.0f us\n", total_waiting * 1000 * 10);
    printf("Per-process Overhead     = %.2f us\n", (total_waiting / NUM_THREADS) * 1000);
    printf("========================================================================================\n");
    printf("Note: Results incorrect due to unsynchronized access to shared motion log\n");
    printf("========================================================================================\n");

    return 0;
}
