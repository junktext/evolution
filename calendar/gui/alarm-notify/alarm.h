#ifndef ALARM_H
#define ALARM_H

#include <time.h>

typedef void (*AlarmFunction)(time_t time, void *closuse);

void alarm_init    (void);
void alarm_add     (time_t alarm_time, AlarmFunction fn, void *closure);
int  alarm_kill    (void *closure);

#endif
