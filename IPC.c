#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define SHM_NAME  "/surveillance_frame"
#define SHM_SIZE  256
#define FRAMES    5

typedef struct {
    int  frame_id;
    char frame_data[240];
} FrameBuffer;

struct timespec transfer_start, transfer_end;

void* writer(void* arg) {
    // shm_open: creates named object in /dev/shm backed by tmpfs
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, SHM_SIZE);

    // mmap: kernel maps physical page into process virtual address space via mm_struct VMA
    FrameBuffer* fb = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    clock_gettime(CLOCK_MONOTONIC, &transfer_start); // Start timer

    for (int i = 1; i <= FRAMES; i++) {
        fb->frame_id = i;
        snprintf(fb->frame_data, sizeof(fb->frame_data),
                 "CAM-1 | Frame-%d | Motion zone: sector-%d", i, i * 2);
        printf("[WRITER] Frame %d written to shared memory\n", i);
        sleep(1);
    }

    munmap(fb, SHM_SIZE);
    close(fd);
    return NULL;
}

void* reader(void* arg) {
    sleep(1);

    // shm_open: opens same named object — kernel maps same physical page
    int fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    FrameBuffer* fb = mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, fd, 0);

    for (int i = 0; i < FRAMES; i++) {
        printf("[READER] Frame %d received: %s\n", fb->frame_id, fb->frame_data);
        sleep(1);
    }

    clock_gettime(CLOCK_MONOTONIC, &transfer_end); // Stop timer

    munmap(fb, SHM_SIZE);
    close(fd);
    shm_unlink(SHM_NAME); // Remove named object from /dev/shm
    return NULL;
}

int main() {
    pthread_t w, r;
    pthread_create(&w, NULL, writer, NULL);
    pthread_create(&r, NULL, reader, NULL);
    pthread_join(w, NULL);
    pthread_join(r, NULL);

    double total_ms = (transfer_end.tv_sec  - transfer_start.tv_sec)  * 1000.0
                    + (transfer_end.tv_nsec - transfer_start.tv_nsec) / 1e6;

    printf("\n[DONE] Frame transfer complete — zero kernel copies involved\n");
    printf("[METRIC] Total transfer time for %d frames: %.2f ms\n", FRAMES, total_ms);
    return 0;
}
