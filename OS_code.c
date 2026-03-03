#include <stdio.h>
#include <unistd.h>
#include <time.h>

#define MAX 10

typedef struct {
    int pid, at, bt, ct, tat, wt, rt;
    int priority;
    int remaining_bt;
    int started;
} Process;

Process p[MAX], backup[MAX];
int n;
int current_time;
float avg_wt, avg_rt;

/* ===== NEW METRICS ===== */
int max_wt, max_tat;
float throughput, cpu_utilization;
clock_t program_start, program_end;

/* ---------- Helpers ---------- */

int max_bt() {
    int max = 0;
    for (int i = 0; i < n; i++)
        if (p[i].bt > max) max = p[i].bt;
    return max;
}

void backup_processes() {
    for (int i = 0; i < n; i++)
        backup[i] = p[i];
}

void restore_processes() {
    for (int i = 0; i < n; i++) {
        p[i] = backup[i];
        p[i].remaining_bt = p[i].bt;
        p[i].started = 0;
        p[i].ct = p[i].tat = p[i].wt = p[i].rt = 0;
    }
}

void calculate_metrics() {
    avg_wt = avg_rt = 0;
    max_wt = max_tat = 0;

    int total_bt = 0;
    int first_at = 1e9, last_ct = 0;

    for (int i = 0; i < n; i++) {
        p[i].tat = p[i].ct - p[i].at;
        p[i].wt = p[i].tat - p[i].bt;

        avg_wt += p[i].wt;
        avg_rt += p[i].rt;

        if (p[i].wt > max_wt) max_wt = p[i].wt;
        if (p[i].tat > max_tat) max_tat = p[i].tat;

        total_bt += p[i].bt;
        if (p[i].at < first_at) first_at = p[i].at;
        if (p[i].ct > last_ct) last_ct = p[i].ct;
    }

    avg_wt /= n;
    avg_rt /= n;

    int makespan = last_ct - first_at;
    throughput = (float)n / makespan;
    cpu_utilization = ((float)total_bt / makespan) * 100;
}

void print_table() {
    printf("\nPNo\tAT\tBT\tCT\tTAT\tWT\tRT\n");
    for (int i = 0; i < n; i++)
        printf("P%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
               p[i].pid, p[i].at, p[i].bt,
               p[i].ct, p[i].tat, p[i].wt, p[i].rt);

    printf("\nAverage Waiting Time  = %.2f\n", avg_wt);
    printf("Average Response Time = %.2f\n", avg_rt);

    /* ===== EXTENDED METRICS ===== */
    printf("Max Waiting Time      = %d\n", max_wt);
    printf("Max Turnaround Time   = %d\n", max_tat);
    printf("Throughput            = %.3f processes/unit\n", throughput);
    printf("CPU Utilization       = %.2f%%\n", cpu_utilization);
    printf("Avg Process Latency   = %.2f\n", avg_rt);
    printf("Worst-case Latency (heuristic) = %d\n", max_bt() * n);
}

/* ---------- Execution Time Printer ---------- */
void print_execution_times(const char* algo_name) {
    printf("\n=== %s TIMINGS ===\n", algo_name);
    for (int i = 0; i < n; i++) {
        int start_time = p[i].ct - p[i].bt;
        printf("P%d -> Start: %d, Termination: %d\n",
               p[i].pid, start_time, p[i].ct);
    }
}

/* ---------- FCFS ---------- */

void fcfs() {
    printf("\n=== FCFS EXECUTION ===\n");
    current_time = 0;

    for (int i = 0; i < n; i++) {
        if (current_time < p[i].at)
            current_time = p[i].at;

        p[i].rt = current_time - p[i].at;
        printf("[Time %d] CPU -> P%d (running %d units)\n",
               current_time, p[i].pid, p[i].bt);

        sleep(p[i].bt);
        current_time += p[i].bt;
        p[i].ct = current_time;
    }
    calculate_metrics();
    print_table();
    print_execution_times("FCFS");
}

/* ---------- SJF ---------- */

void sjf() {
    printf("\n=== SJF EXECUTION ===\n");
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
        printf("[Time %d] CPU -> P%d (running %d units)\n",
               current_time, p[idx].pid, p[idx].bt);

        sleep(p[idx].bt);
        current_time += p[idx].bt;
        p[idx].ct = current_time;
        p[idx].remaining_bt = 0;
        completed++;
    }
    calculate_metrics();
    print_table();
    print_execution_times("SJF");
}

/* ---------- Priority ---------- */

void priority_scheduling() {
    printf("\n=== PRIORITY SCHEDULING EXECUTION ===\n");
    int completed = 0;
    current_time = 0;

    while (completed < n) {
        int idx = -1, best = 9999;

        for (int i = 0; i < n; i++) {
            if (p[i].remaining_bt > 0 &&
                p[i].at <= current_time &&
                p[i].priority < best) {
                best = p[i].priority;
                idx = i;
            }
        }

        if (idx == -1) {
            current_time++;
            continue;
        }

        p[idx].rt = current_time - p[idx].at;
        printf("[Time %d] CPU -> P%d (Priority %d, %d units)\n",
               current_time, p[idx].pid, p[idx].priority, p[idx].bt);

        sleep(p[idx].bt);
        current_time += p[idx].bt;
        p[idx].ct = current_time;
        p[idx].remaining_bt = 0;
        completed++;
    }
    calculate_metrics();
    print_table();
    print_execution_times("PRIORITY");
}

/* ---------- Round Robin ---------- */

void round_robin(int tq) {
    printf("\n=== ROUND ROBIN EXECUTION ===\n");
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

                printf("[Time %d] CPU -> P%d (running %d units)\n",
                       current_time, p[i].pid, exec);

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
    print_table();
    print_execution_times("ROUND ROBIN");
}

/* ---------- Main ---------- */

int main() {
    int tq = 2;

    program_start = clock();

    printf("Enter number of processes: ");
    scanf("%d", &n);

    for (int i = 0; i < n; i++) {
        p[i].pid = i + 1;
        printf("\nProcess %d Arrival Time: ", p[i].pid);
        scanf("%d", &p[i].at);
        printf("Process %d Burst Time: ", p[i].pid);
        scanf("%d", &p[i].bt);
        printf("Process %d Priority: ", p[i].pid);
        scanf("%d", &p[i].priority);
        p[i].remaining_bt = p[i].bt;
        p[i].started = 0;
    }

    backup_processes();

    restore_processes(); fcfs();
    restore_processes(); sjf();
    restore_processes(); priority_scheduling();
    restore_processes(); round_robin(tq);

    program_end = clock();
    printf("\nProgram Execution Time = %.6f seconds\n",
           (double)(program_end - program_start) / CLOCKS_PER_SEC);

    return 0;
}
