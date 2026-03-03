#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define MAX 10

typedef struct {
    int pid;
    int at;
    int bt;
    int ct;
    int tat;
    int wt;
    int rt;
    int priority;
    int remaining_bt;
    int started;
} Process;

Process p[MAX];
int n;
int current_time = 0;
float avg_wt = 0, avg_rt = 0;

/* ---------- Utility ---------- */

void reset_processes() {
    for (int i = 0; i < n; i++) {
        p[i].remaining_bt = p[i].bt;
        p[i].started = 0;
        p[i].ct = p[i].tat = p[i].wt = p[i].rt = 0;
    }
}

void calculate_metrics() {
    avg_wt = avg_rt = 0;
    for (int i = 0; i < n; i++) {
        p[i].tat = p[i].ct - p[i].at;
        p[i].wt  = p[i].tat - p[i].bt;
        avg_wt += p[i].wt;
        avg_rt += p[i].rt;
    }
    avg_wt /= n;
    avg_rt /= n;
}

void print_table() {
    printf("\n---------------------------------------------------------\n");
    printf("PNo\tAT\tBT\tCT\tTAT\tWT\tRT\n");
    printf("---------------------------------------------------------\n");
    for (int i = 0; i < n; i++) {
        printf("P%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
               p[i].pid, p[i].at, p[i].bt,
               p[i].ct, p[i].tat, p[i].wt, p[i].rt);
    }
    printf("---------------------------------------------------------\n");
    printf("Average Waiting Time  = %.2f\n", avg_wt);
    printf("Average Response Time = %.2f\n", avg_rt);
}

/* ---------- FCFS ---------- */

void fcfs() {
    current_time = 0;
    for (int i = 0; i < n; i++) {
        if (current_time < p[i].at)
            current_time = p[i].at;

        p[i].rt = current_time - p[i].at;
        sleep(p[i].bt);
        current_time += p[i].bt;
        p[i].ct = current_time;
    }
    calculate_metrics();
}

/* ---------- SJF (Non-Preemptive) ---------- */

void sjf() {
    int completed = 0;
    current_time = 0;

    while (completed < n) {
        int idx = -1, min_bt = 9999;

        for (int i = 0; i < n; i++) {
            if (p[i].remaining_bt > 0 &&
                p[i].at <= current_time &&
                p[i].bt < min_bt) {
                min_bt = p[i].bt;
                idx = i;
            }
        }

        if (idx == -1) {
            current_time++;
            continue;
        }

        p[idx].rt = current_time - p[idx].at;
        sleep(p[idx].bt);
        current_time += p[idx].bt;
        p[idx].ct = current_time;
        p[idx].remaining_bt = 0;
        completed++;
    }
    calculate_metrics();
}

/* ---------- Priority Scheduling ---------- */

void priority_scheduling() {
    int completed = 0;
    current_time = 0;

    while (completed < n) {
        int idx = -1, high = 9999;

        for (int i = 0; i < n; i++) {
            if (p[i].remaining_bt > 0 &&
                p[i].at <= current_time &&
                p[i].priority < high) {
                high = p[i].priority;
                idx = i;
            }
        }

        if (idx == -1) {
            current_time++;
            continue;
        }

        p[idx].rt = current_time - p[idx].at;
        sleep(p[idx].bt);
        current_time += p[idx].bt;
        p[idx].ct = current_time;
        p[idx].remaining_bt = 0;
        completed++;
    }
    calculate_metrics();
}

/* ---------- Round Robin ---------- */

void round_robin(int tq) {
    int completed = 0;
    current_time = 0;

    while (completed < n) {
        int idle = 1;

        for (int i = 0; i < n; i++) {
            if (p[i].remaining_bt > 0 && p[i].at <= current_time) {
                idle = 0;

                if (!p[i].started) {
                    p[i].rt = current_time - p[i].at;
                    p[i].started = 1;
                }

                int exec = (p[i].remaining_bt > tq) ? tq : p[i].remaining_bt;
                sleep(exec);
                current_time += exec;
                p[i].remaining_bt -= exec;

                if (p[i].remaining_bt == 0) {
                    p[i].ct = current_time;
                    completed++;
                }
            }
        }
        if (idle) current_time++;
    }
    calculate_metrics();
}

/* ---------- Main ---------- */

int main() {
    int choice, tq;

    printf("Enter number of processes: ");
    scanf("%d", &n);

    for (int i = 0; i < n; i++) {
        p[i].pid = i + 1;
        printf("\nProcess %d Arrival Time: ", p[i].pid);
        scanf("%d", &p[i].at);
        printf("Process %d Burst Time: ", p[i].pid);
        scanf("%d", &p[i].bt);
        printf("Process %d Priority (lower = higher): ", p[i].pid);
        scanf("%d", &p[i].priority);
    }

    reset_processes();

    printf("\n1. FCFS\n2. SJF\n3. Priority Scheduling\n4. Round Robin\nChoose: ");
    scanf("%d", &choice);

    switch (choice) {
        case 1: fcfs(); break;
        case 2: sjf(); break;
        case 3: priority_scheduling(); break;
        case 4:
            printf("Enter Time Quantum: ");
            scanf("%d", &tq);
            round_robin(tq);
            break;
        default:
            printf("Invalid choice\n");
            return 0;
    }

    print_table();
    return 0;
}
