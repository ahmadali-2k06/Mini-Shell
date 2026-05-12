#include "gantt.h"
#include <stdio.h>

void render_gantt(const schedule_result_t *result) {
    if (!result || result->entry_count == 0) {
        printf("No schedule has been run yet or timeline is empty.\n");
        return;
    }

    printf("\nGantt Chart:\n");
    
    // Top border
    printf("  ");
    for (int i = 0; i < result->entry_count; i++) {
        printf("-------");
    }
    printf("\n");

    // Job labels
    printf("  ");
    for (int i = 0; i < result->entry_count; i++) {
        printf("|  %s  ", result->entries[i].label);
    }
    printf("|\n");

    // Bottom border
    printf("  ");
    for (int i = 0; i < result->entry_count; i++) {
        printf("-------");
    }
    printf("\n");

    // Timeline numbers
    printf("  ");
    for (int i = 0; i < result->entry_count; i++) {
        printf("%-7d", result->entries[i].start);
    }
    // Print the final end time
    printf("%d\n", result->entries[result->entry_count - 1].end);
}

void print_stats(const job_stats_t *stats, int count) {
    if (!stats || count == 0) return;

    printf("\nJob Statistics:\n");
    printf("%-5s | %-15s | %-15s | %-15s | %-15s\n", "ID", "Command", "Turnaround", "Waiting", "Response");
    printf("----------------------------------------------------------------------\n");

    double total_waiting = 0;
    
    for (int i = 0; i < count; i++) {
        printf("[%-3d] | %-15s | %-15d | %-15d | %-15d\n", 
            stats[i].job_id, 
            stats[i].command, 
            stats[i].turnaround_time, 
            stats[i].waiting_time, 
            stats[i].response_time);
            
        total_waiting += stats[i].waiting_time;
    }

    printf("\nAverage Waiting Time: %.2f seconds\n", total_waiting / count);
}
