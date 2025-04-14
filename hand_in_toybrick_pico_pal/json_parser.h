#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <stdbool.h>

#define MAX_SCHEDULES 10
#define MAX_FIELD_LEN 256

typedef struct {
    int id;
    char name[50];
    bool is_repeated;
    char repeat_type[10];  // "day"/"week"/"month"
    int which_day;         // 1-7(周), 1-31(月)
    char start_date[20];   // YYYY-MM-DD (重复任务)
    char end_date[20];     // YYYY-MM-DD (重复任务)
    char exact_date[20];   // YYYY-MM-DD (非重复任务) ← 新增字段
    int reminder_count;
    int reminder_interval; // 首次提醒提前分钟数
    char exact_time[6];    // HH:MM
} ScheduleItem;

extern ScheduleItem schedules[MAX_SCHEDULES];
extern int schedule_count;

bool parse_json(const char *json_data);

#endif