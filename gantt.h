#ifndef GANTT_H
#define GANTT_H

typedef struct {
    int   job_id;       // Which job ran during this slot
    char  label[8];     // "P1", "P2", etc. for display
    int   start;        // Start time offset (seconds from schedule start)
    int   end;          // End time offset
} gantt_entry_t;

typedef struct {
    gantt_entry_t  entries[256];  // Gantt chart slots
    int            entry_count;   // How many slots were used
    int            total_time;    // Total schedule duration in seconds
} schedule_result_t;

typedef struct {
    int   job_id;
    char  command[256];
    int   turnaround_time;  // finish_time - arrival_time
    int   waiting_time;     // turnaround_time - burst_time
    int   response_time;    // start_time - arrival_time
} job_stats_t;

void render_gantt(const schedule_result_t *result);
void print_stats(const job_stats_t *stats, int count);

#endif // GANTT_H
