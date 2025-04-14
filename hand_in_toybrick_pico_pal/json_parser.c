#include "json_parser.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

ScheduleItem schedules[MAX_SCHEDULES];
int schedule_count = 0;

// 跳过空白字符
static const char* skip_whitespace(const char *p) {
    while (*p && isspace(*p)) p++;
    return p;
}

// 解析字符串
static const char* parse_string(const char *p, char *output) {
    p = skip_whitespace(p);
    if (*p != '"') return NULL;
    p++;
    
    int i = 0;
    while (*p && *p != '"' && i < MAX_FIELD_LEN-1) {
        if (*p == '\\') p++; // 简单处理转义字符
        output[i++] = *p++;
    }
    output[i] = '\0';
    return (*p == '"') ? p + 1 : NULL;
}

// 解析数字
static const char* parse_number(const char *p, int *output) {
    p = skip_whitespace(p);
    char *end;
    *output = strtol(p, (char**)&end, 10);
    return end;
}

// 解析布尔值
static const char* parse_boolean(const char *p, bool *output) {
    p = skip_whitespace(p);
    if (strncmp(p, "true", 4) == 0) {
        *output = true;
        return p + 4;
    } else if (strncmp(p, "false", 5) == 0) {
        *output = false;
        return p + 5;
    }
    return NULL;
}

// 解析null值
static const char* parse_null(const char *p) {
    p = skip_whitespace(p);
    if (strncmp(p, "null", 4) == 0) return p + 4;
    return NULL;
}

// 解析单个日程项
static const char* parse_schedule_item(const char *p, ScheduleItem *item) {
    p = skip_whitespace(p);
    if (*p != '{') return NULL;
    p++;
    
    while (*p && *p != '}') {
        p = skip_whitespace(p);
        
        // 解析key
        char key[MAX_FIELD_LEN];
        p = parse_string(p, key);
        if (!p) return NULL;
        
        p = skip_whitespace(p);
        if (*p != ':') return NULL;
        p++;
        p = skip_whitespace(p);
        
        // 根据key解析value
        if (strcmp(key, "id") == 0) {
            p = parse_number(p, &item->id);
        } else if (strcmp(key, "name") == 0) {
            p = parse_string(p, item->name);
        } else if (strcmp(key, "is_repeated") == 0) {
            p = parse_boolean(p, &item->is_repeated);
        } else if (strcmp(key, "repeat_type") == 0) {
            if (*p == 'n') {
                p = parse_null(p);
                strcpy(item->repeat_type, "");
            } else {
                p = parse_string(p, item->repeat_type);
            }
        } else if (strcmp(key, "which_day") == 0) {
            if (*p == 'n') {
                p = parse_null(p);
                item->which_day = -1;
            } else {
                p = parse_number(p, &item->which_day);
            }
        } else if (strcmp(key, "start_date") == 0) {
            if (*p == 'n') {
                p = parse_null(p);
                strcpy(item->start_date, "");
            } else {
                p = parse_string(p, item->start_date);
            }
        } else if (strcmp(key, "end_date") == 0) {
            if (*p == 'n') {
                p = parse_null(p);
                strcpy(item->end_date, "");
            } else {
                p = parse_string(p, item->end_date);
            }
        } else if (strcmp(key, "exact_date") == 0) {
            if (*p == 'n') {
                p = parse_null(p);
                strcpy(item->exact_date, "");
            } else {
                p = parse_string(p, item->exact_date);
            }
        } else if (strcmp(key, "reminder_count") == 0) {
            p = parse_number(p, &item->reminder_count);
        } else if (strcmp(key, "reminder_interval") == 0) {
            p = parse_number(p, &item->reminder_interval);
        } else if (strcmp(key, "exact_time") == 0) {
            p = parse_string(p, item->exact_time);
        }
        
        if (!p) return NULL;
        p = skip_whitespace(p);
        if (*p == ',') p++;
    }
    
    if (*p != '}') return NULL;
    return p + 1;
}

// 主解析函数
bool parse_json(const char *json_data) {
    const char *p = json_data;
    p = skip_whitespace(p);
    
    if (*p != '{') return false;
    p++;
    
    while (*p && *p != '}') {
        p = skip_whitespace(p);
        
        char key[MAX_FIELD_LEN];
        p = parse_string(p, key);
        if (!p) return false;
        
        p = skip_whitespace(p);
        if (*p != ':') return false;
        p++;
        p = skip_whitespace(p);
        
        if (strcmp(key, "schedule_count") == 0) {
            p = parse_number(p, &schedule_count);
        } else if (strcmp(key, "schedules") == 0) {
            if (*p != '[') return false;
            p++;
            
            int i = 0;
            while (*p && *p != ']' && i < MAX_SCHEDULES) {
                p = parse_schedule_item(p, &schedules[i]);
                if (!p) return false;
                i++;
                
                p = skip_whitespace(p);
                if (*p == ',') p++;
            }
            
            if (*p != ']') return false;
            p++;
        }
        
        if (!p) return false;
        p = skip_whitespace(p);
        if (*p == ',') p++;
    }
    
    return true;
}