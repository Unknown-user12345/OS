#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

int frame_buffer = 0;
int alert_count  = 0;

void* write_frame(void* id) {
    int val = frame_buffer;   // Read — other threads can read same value here
    usleep(100);              // Wide gap — almost guarantees overlap
    val++;
    frame_buffer = val;       // Multiple threads write same incremented value
    printf("[CAM-%d]     Frame written. Buffer: %d\n", *(int*)id, frame_buffer);
    return NULL;
}

void* trigger_alert(void* id) {
    alert_count++;
    printf("[TRIGGER-%d] Alert added.  Queue:  %d\n", *(int*)id, alert_count);
    return NULL;
}

int main() {
    pthread_t cam[20], alert[10];
    int cam_ids[20], alert_ids[10];

    for (int i = 0; i < 20; i++) { cam_ids[i]  = i+1; pthread_create(&cam[i],   NULL, write_frame,   &cam_ids[i]);   }
    for (int i = 0; i < 10; i++) { alert_ids[i] = i+1; pthread_create(&alert[i], NULL, trigger_alert, &alert_ids[i]); }

    for (int i = 0; i < 20; i++) pthread_join(cam[i], NULL);
    for (int i = 0; i < 10; i++) pthread_join(alert[i], NULL);

    printf("\nFrame buffer — Expected: 20  | Actual: %d  (lost writes from race)\n", frame_buffer);
    printf("Alert queue  — Expected: = 3 | Actual: %d  (queue flooded)\n", alert_count);
    return 0;
}
