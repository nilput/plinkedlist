#include <unistd.h> //timer works only on linux (because of clock_gettime())
#include <time.h> 
#include <stdio.h>
#include <stdbool.h>

struct timer_info{
    bool stopped;
    struct timespec tstart;
};
void timer_begin(struct timer_info *timer){
    clock_gettime(CLOCK_MONOTONIC, &timer->tstart);
}
double timer_dt(struct timer_info *timer){
    struct timespec tend; 
    clock_gettime(CLOCK_MONOTONIC, &tend);
    return ((double)tend.tv_sec          + 1.0e-9 * tend.tv_nsec) - 
           ((double)timer->tstart.tv_sec + 1.0e-9 * timer->tstart.tv_nsec);
}

