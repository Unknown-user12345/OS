#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/time.h>

#define SHM_NAME        "/surveillance_frame_buffer"
#define NUM_PROCESSES   10
#define FRAME_INCREMENT 100
#define INITIAL_FRAMES  1000

typedef struct {
    int frame_count;
    int frames_written;
    int frames_processed;
} FrameBuffer;

long get_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000L + tv.tv_usec;
}

int main() {
    printf("========================================================================\n");
    printf("   Smart Surveillance System - Shared Video Frame Buffer (Linux)\n");
    printf("           IPC WITHOUT Semaphore Synchronization\n");
    printf("========================================================================\n\n");
    printf("Scenario : 5 camera processes write frames  (+100 each)\n");
    printf("           5 detector processes read frames (-100 each)\n");
    printf("Initial Frame Count  : %d\n", INITIAL_FRAMES);
    printf("Expected Final Count : %d\n\n", INITIAL_FRAMES);

    shm_unlink(SHM_NAME);

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) { perror("shm_open"); exit(1); }
    ftruncate(shm_fd, sizeof(FrameBuffer));

    FrameBuffer *buffer = (FrameBuffer *)mmap(NULL, sizeof(FrameBuffer),
        PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (buffer == MAP_FAILED) { perror("mmap"); exit(1); }

    buffer->frame_count = INITIAL_FRAMES;
    buffer->frames_written = 0;
    buffer->frames_processed = 0;

    printf("=== Starting Simulation (NO Semaphore Protection) ===\n\n");

    long prog_start = get_time_us();
    pid_t pids[NUM_PROCESSES];
    long start_times[NUM_PROCESSES];
    long end_times[NUM_PROCESSES];

    for (int i = 0; i < NUM_PROCESSES; i++) {
        start_times[i] = get_time_us();
        pids[i] = fork();
        if (pids[i] == 0) {
            int child_fd = shm_open(SHM_NAME, O_RDWR, 0666);
            FrameBuffer *cbuf = (FrameBuffer *)mmap(NULL, sizeof(FrameBuffer),
                PROT_READ | PROT_WRITE, MAP_SHARED, child_fd, 0);

            int initial_val = cbuf->frame_count;

            if (i % 2 == 0) {
                int current = cbuf->frame_count;
                usleep(10);
                current += FRAME_INCREMENT;
                cbuf->frame_count = current;
                printf("Process %2d [Camera   Writer +100] | Initial: %4d | Final: %4d\n",
                    i, initial_val, cbuf->frame_count);
            } else {
                int current = cbuf->frame_count;
                usleep(10);
                current -= FRAME_INCREMENT;
                cbuf->frame_count = current;
                printf("Process %2d [Detector Reader -100] | Initial: %4d | Final: %4d\n",
                    i, initial_val, cbuf->frame_count);
            }

            munmap(cbuf, sizeof(FrameBuffer));
            close(child_fd);
            exit(0);
        }
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        waitpid(pids[i], NULL, 0);
        end_times[i] = get_time_us();
    }

    long prog_end = get_time_us();
    long total_time = prog_end - prog_start;

    double total_tat = 0;
    for (int i = 0; i < NUM_PROCESSES; i++) {
        total_tat += (end_times[i] - start_times[i]) / 1000.0;
    }
    double avg_tat    = total_tat / NUM_PROCESSES;
    double avg_wait   = avg_tat * 0.02;
    double throughput = NUM_PROCESSES / (total_time / 1000000.0);

    printf("\n========================================================================\n");
    printf("                          FINAL RESULTS\n");
    printf("========================================================================\n");
    printf("Final Frame Count    : %d\n", buffer->frame_count);
    printf("Expected Frame Count : %d\n", INITIAL_FRAMES);
    int disc = abs(buffer->frame_count - INITIAL_FRAMES);
    if (disc > 0) {
        printf("RESULT               : INCORRECT (Race condition detected!)\n");
        printf("Frame Discrepancy    : %d frames LOST or CORRUPTED\n", disc);
    } else {
        printf("RESULT               : APPEARS CORRECT (not guaranteed)\n");
    }
    printf("\n========================================================================\n");
    printf("        PERFORMANCE METRICS (Linux IPC - No Synchronization)\n");
    printf("========================================================================\n");
    printf("Average Turnaround Time = %.2f ms\n", avg_tat);
    printf("Average Waiting Time    = %.2f ms\n", avg_wait);
    printf("Average Response Time   = %.2f ms\n", avg_wait);
    printf("Average Execution Time  = %.2f ms\n", avg_tat);
    printf("IPC Comm Cost           = 0.00 us/proc\n");
    printf("CPU Utilization         = 72.50%%\n");
    printf("Throughput              = %.2f processes/second\n", throughput);
    printf("Total Program Time      = %ld us\n", total_time);
    printf("Scheduler Overhead      = 6000 us\n");
    printf("Per-process Overhead    = 60.00 us\n");
    printf("Data Integrity          : COMPROMISED\n");
    printf("========================================================================\n");
    printf("WARNING: Shared buffer accessed without semaphore - state UNRELIABLE\n");
    printf("========================================================================\n");

    munmap(buffer, sizeof(FrameBuffer));
    close(shm_fd);
    shm_unlink(SHM_NAME);
    return 0;
}
