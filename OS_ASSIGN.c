#include <stdio.h>
#include <math.h>
#include <time.h>

void cpu_stress_test() {
    volatile double result = 0.0;
    for (long i = 0; i < 1000000000; i++) {
        result += sqrt(i);  // Perform some CPU intensive calculations
    }
    printf("Result: %f\n", result);
}

int main() {
    printf("Starting CPU Stress Test...\n");
    clock_t start_time = clock();  // Start timing
    cpu_stress_test();
    clock_t end_time = clock();  // End timing
    double time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("CPU stress test completed in %.2f seconds.\n", time_taken);
    return 0;
}
