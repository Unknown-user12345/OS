#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define MAX_ALERTS 3

int frame_buffer = 0;
int alert_count  = 0;
pthread_mutex_t frame_lock = PTHREAD_MUTEX_INITIALIZER;
sem_t           alert_slots;

void* write_frame(void* id) {
    pthread_mutex_lock(&frame_lock);
    int val = frame_buffer;
    usleep(100);              // Same delay — mutex still guarantees correctness
    val++;
    frame_buffer = val;
    printf("[CAM-%d]     Frame written. Buffer: %d\n", *(int*)id, frame_buffer);
    pthread_mutex_unlock(&frame_lock);
    return NULL;
}

void* trigger_alert(void* id) {
    if (sem_trywait(&alert_slots) == 0) {
        alert_count++;
        printf("[TRIGGER-%d] Alert queued.  Queue:  %d\n", *(int*)id, alert_count);
    } else {
        printf("[TRIGGER-%d] DROPPED — queue full, DoS blocked\n", *(int*)id);
    }
    return NULL;
}

int main() {
    pthread_t cam[20], alert[10];
    int cam_ids[20], alert_ids[10];

    sem_init(&alert_slots, 0, MAX_ALERTS);

    for (int i = 0; i < 20; i++) { cam_ids[i]  = i+1; pthread_create(&cam[i],   NULL, write_frame,   &cam_ids[i]);   }
    for (int i = 0; i < 10; i++) { alert_ids[i] = i+1; pthread_create(&alert[i], NULL, trigger_alert, &alert_ids[i]); }

    for (int i = 0; i < 20; i++) pthread_join(cam[i], NULL);
    for (int i = 0; i < 10; i++) pthread_join(alert[i], NULL);

    printf("\nFrame buffer — Expected: 20  | Actual: %d  (race eliminated)\n", frame_buffer);
    printf("Alert queue  — Expected: = 3 | Actual: %d  (flood contained)\n", alert_count);

    pthread_mutex_destroy(&frame_lock);
    sem_destroy(&alert_slots);
    return 0;
}
