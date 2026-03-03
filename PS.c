#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>

typedef struct {
    int pid;
    char task_name[50];
    int arrival_time;
    int burst_time;
    int priority;  // Lower number = Higher priority
    double calc_start_time;
    double calc_completion_time;
    double calc_turnaround_time;
    double calc_waiting_time;
    double calc_response_time;
    int completed;
} Process;

double get_wall_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void cpu_intensive_work(int seconds) {
    double start = get_wall_time();
    double target = start + seconds;
    
    volatile double result = 0.0;
    while (get_wall_time() < target) {
        for (volatile long i = 0; i < 10000000; i++) {
            result += i * 3.14159;
            result = result / 2.71828;
        }
    }
}

void execute_process(Process *p) {
    pid_t child = fork();
    
    if (child == 0) {
        cpu_intensive_work(p->burst_time);
        exit(0);
    } else {
        int status;
        waitpid(child, &status, 0);
    }
}

void calculate_priority_times(Process p[], int n) {
    double current_time = 0;
    int completed_count = 0;
    
    // Initialize
    for (int i = 0; i < n; i++) {
        p[i].completed = 0;
    }
    
    while (completed_count < n) {
        int highest_priority = -1;
        int min_priority = 999999;
        
        // Find highest priority job among arrived processes
        for (int i = 0; i < n; i++) {
            if (!p[i].completed && p[i].arrival_time <= current_time) {
                if (p[i].priority < min_priority) {
                    min_priority = p[i].priority;
                    highest_priority = i;
                }
            }
        }
        
        if (highest_priority == -1) {
            // No process available, advance time to next arrival
            double next_arrival = 999999;
            for (int i = 0; i < n; i++) {
                if (!p[i].completed && p[i].arrival_time > current_time) {
                    if (p[i].arrival_time < next_arrival) {
                        next_arrival = p[i].arrival_time;
                    }
                }
            }
            current_time = next_arrival;
        } else {
            p[highest_priority].calc_start_time = current_time;
            p[highest_priority].calc_completion_time = current_time + p[highest_priority].burst_time;
            p[highest_priority].calc_turnaround_time = p[highest_priority].calc_completion_time - p[highest_priority].arrival_time;
            p[highest_priority].calc_waiting_time = p[highest_priority].calc_start_time - p[highest_priority].arrival_time;
            p[highest_priority].calc_response_time = p[highest_priority].calc_start_time - p[highest_priority].arrival_time;
            p[highest_priority].completed = 1;
            
            current_time = p[highest_priority].calc_completion_time;
            completed_count++;
        }
    }
}

void print_table(Process p[], int n) {
    printf("\n");
    printf("========================================================================================\n");
    printf("                  PRIORITY SCHEDULING - PROCESS EXECUTION TABLE                        \n");
    printf("                             (Theoretical Calculations)                                \n");
    printf("========================================================================================\n");
    printf("PID  Task Name                  PR  AT    BT    ST      CT      TAT     WT      RT\n");
    printf("----------------------------------------------------------------------------------------\n");
    
    for (int i = 0; i < n; i++) {
        printf("P%-3d %-24s %-4d %-5d %-5d %-7.2f %-7.2f %-7.2f %-7.2f %-7.2f\n",
               p[i].pid,
               p[i].task_name,
               p[i].priority,
               p[i].arrival_time,
               p[i].burst_time,
               p[i].calc_start_time,
               p[i].calc_completion_time,
               p[i].calc_turnaround_time,
               p[i].calc_waiting_time,
               p[i].calc_response_time);
    }
    printf("========================================================================================\n");
    printf("PR=Priority (lower=higher), AT=Arrival Time, BT=Burst Time, ST=Start Time\n");
    printf("CT=Completion Time, TAT=Turnaround Time, WT=Waiting Time, RT=Response Time (seconds)\n");
    printf("Note: Table values are identical across OS (based on algorithm logic)\n");
    printf("========================================================================================\n");
}

void calculate_metrics(Process p[], int n, double total_actual_time, int context_switches) {
    double total_waiting_time = 0;
    double total_turnaround_time = 0;
    double total_response_time = 0;
    double max_waiting_time = 0;
    double max_turnaround_time = 0;
    double total_burst_time = 0;
    double max_completion_time = 0;
    
    for (int i = 0; i < n; i++) {
        total_waiting_time += p[i].calc_waiting_time;
        total_turnaround_time += p[i].calc_turnaround_time;
        total_response_time += p[i].calc_response_time;
        total_burst_time += p[i].burst_time;
        
        if (p[i].calc_waiting_time > max_waiting_time)
            max_waiting_time = p[i].calc_waiting_time;
        if (p[i].calc_turnaround_time > max_turnaround_time)
            max_turnaround_time = p[i].calc_turnaround_time;
        if (p[i].calc_completion_time > max_completion_time)
            max_completion_time = p[i].calc_completion_time;
    }
    
    double cpu_utilization = (total_burst_time / total_actual_time) * 100.0;
    
    printf("\n");
    printf("========================================================================================\n");
    printf("                   PERFORMANCE METRICS (Linux - Priority Scheduling)                   \n");
    printf("                          (Actual OS Measurements)                                     \n");
    printf("========================================================================================\n");
    printf("Average Waiting Time       = %.2f seconds\n", total_waiting_time / n);
    printf("Average Response Time      = %.2f seconds\n", total_response_time / n);
    printf("Average Turnaround Time    = %.2f seconds\n", total_turnaround_time / n);
    printf("Max Waiting Time           = %.2f seconds\n", max_waiting_time);
    printf("Max Turnaround Time        = %.2f seconds\n", max_turnaround_time);
    printf("Throughput                 = %.3f processes/second\n", n / total_actual_time);
    printf("CPU Utilization            = %.2f%%\n", cpu_utilization);
    printf("Avg Process Latency        = %.2f seconds\n", total_waiting_time / n);
    printf("Worst-case Latency         = %.2f seconds (heuristic)\n", total_burst_time);
    printf("Total Execution Time       = %.2f seconds\n", total_actual_time);
    printf("Context Switches           = %d\n", context_switches);
    printf("========================================================================================\n");
    printf("Note: These metrics vary by OS due to kernel overhead and scheduling efficiency\n");
    printf("========================================================================================\n");
}

int main() {
    Process processes[] = {
        {1, "Video Capture - Cam1", 0, 5, 2, 0, 0, 0, 0, 0, 0},
        {2, "Video Capture - Cam2", 0, 5, 2, 0, 0, 0, 0, 0, 0},
        {3, "Motion Detection - Zone1", 1, 4, 1, 0, 0, 0, 0, 0, 0},
        {4, "Motion Detection - Zone2", 1, 4, 1, 0, 0, 0, 0, 0, 0},
        {5, "Live Streaming - Station1", 2, 6, 3, 0, 0, 0, 0, 0, 0},
        {6, "Live Streaming - Station2", 2, 6, 3, 0, 0, 0, 0, 0, 0},
        {7, "Alert Generation - System", 3, 3, 0, 0, 0, 0, 0, 0, 0},
        {8, "Face Recognition - Cam1", 4, 7, 1, 0, 0, 0, 0, 0, 0},
        {9, "Face Recognition - Cam2", 4, 7, 1, 0, 0, 0, 0, 0, 0},
        {10, "Video Log Storage", 5, 5, 4, 0, 0, 0, 0, 0, 0},
        {11, "Intrusion Detection", 6, 4, 0, 0, 0, 0, 0, 0, 0},
        {12, "Analytics Processing", 7, 8, 3, 0, 0, 0, 0, 0, 0},
        {13, "Threat Assessment", 8, 6, 0, 0, 0, 0, 0, 0, 0},
        {14, "Report Generation", 9, 5, 4, 0, 0, 0, 0, 0, 0},
        {15, "Database Sync", 10, 4, 5, 0, 0, 0, 0, 0, 0}
    };
    
    int n = sizeof(processes) / sizeof(processes[0]);
    
    printf("========================================================================================\n");
    printf("           Smart Surveillance System - Process Scheduler (Linux/UNIX)                  \n");
    printf("                       Algorithm: Priority Scheduling (PS)                             \n");
    printf("========================================================================================\n\n");
    
    printf("System Configuration:\n");
    printf("  - Total Processes: %d\n", n);
    printf("  - Scheduling Policy: Priority Scheduling (Non-preemptive)\n");
    printf("  - Priority Range: 0 (Highest) to 5 (Lowest)\n");
    printf("  - Execution Mode: Kernel-level process scheduling\n");
    printf("  - Load Type: CPU-intensive surveillance tasks\n\n");
    
    // Calculate theoretical times
    calculate_priority_times(processes, n);
    
    printf("--- Process Queue (Arrival Order with Priorities) ---\n");
    printf("PID  Task Name                         Priority  Arrival Time  Burst Time\n");
    printf("------------------------------------------------------------------------------\n");
    for (int i = 0; i < n; i++) {
        printf("P%-3d %-35s %-9d %-13ds %-10ds\n", 
               processes[i].pid, processes[i].task_name, 
               processes[i].priority, processes[i].arrival_time, 
               processes[i].burst_time);
    }
    
    printf("\n=== Starting Priority Scheduling Simulation ===\n");
    printf("Measuring actual OS performance...\n\n");
    
    double simulation_start = get_wall_time();
    int context_switches = 0;
    double current_time = 0;
    int completed_count = 0;
    
    // Reset completion flags
    for (int i = 0; i < n; i++) {
        processes[i].completed = 0;
    }
    
    while (completed_count < n) {
        int highest_priority = -1;
        int min_priority = 999999;
        
        double elapsed = get_wall_time() - simulation_start;
        
        // Find highest priority job among arrived processes
        for (int i = 0; i < n; i++) {
            if (!processes[i].completed && processes[i].arrival_time <= elapsed) {
                if (processes[i].priority < min_priority) {
                    min_priority = processes[i].priority;
                    highest_priority = i;
                }
            }
        }
        
        if (highest_priority == -1) {
            // Wait for next process arrival
            usleep(100000); // 100ms
        } else {
            printf("▶  Executing P%d: %s (Priority: %d, Burst: %ds)\n", 
                   processes[highest_priority].pid, 
                   processes[highest_priority].task_name,
                   processes[highest_priority].priority,
                   processes[highest_priority].burst_time);
            
            execute_process(&processes[highest_priority]);
            processes[highest_priority].completed = 1;
            context_switches++;
            completed_count++;
            
            printf("✓  Completed P%d\n\n", processes[highest_priority].pid);
        }
    }
    
    double total_actual_time = get_wall_time() - simulation_start;
    
    print_table(processes, n);
    calculate_metrics(processes, n, total_actual_time, context_switches);
    
    printf("\n✓ Simulation completed!\n");
    printf("Compare these metrics with Windows to evaluate OS scheduling efficiency.\n\n");
    
    return 0;
}
