#include "schedule_ui.h"
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/ioctl.h>
//#define _XOPEN_SOURCE  // 启用strptime

int selected_index = 0;
bool running = true;

/* 界面显示函数 */
void clear_screen() { printf("\033[2J\033[H"); }

static void get_current_datetime(char *date_str, char *time_str) {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    strftime(date_str, 11, "%Y-%m-%d", tm);
    strftime(time_str, 6, "%H:%M", tm);
}

static const char* get_schedule_status(ScheduleItem *item) {
    char current_date[11];
    get_current_datetime(current_date, NULL);
    
    if (strcmp(current_date, item->start_date) < 0) return "未开始";
    if (item->end_date[0] && strcmp(current_date, item->end_date) > 0) return "已结束";
    return "进行中";
}

void show_list() {
    clear_screen();
    printf("=== 日程列表 (方向键导航/回车查看详情) ===\n\n");
    printf("ID  名称            时间      状态\n");
    printf("------------------------------------\n");
    
    for (int i = 0; i < schedule_count; i++) {
        printf("%s%-3d %-15s %-8s %-8s\n", 
              (i == selected_index) ? "> " : "  ",
              schedules[i].id,
              schedules[i].name,
              schedules[i].exact_time,
              get_schedule_status(&schedules[i]));
    }
}

void show_detail(int index) {
    clear_screen();
    ScheduleItem *item = &schedules[index];
    
    printf("=== 日程详情 ===\n\n");
    printf("ID: %d\n名称: %s\n", item->id, item->name);
    printf("时间: %s\n类型: %s\n", item->exact_time, 
          item->is_repeated ? "重复" : "单次");
    
    if (item->is_repeated) {
        switch(item->repeat_type[0]) {
            case 'w': 
                printf("每周: 周%d\n", item->which_day); 
                break;
            case 'm': 
                printf("每月: %d号\n", item->which_day);
                break;
        }
    }
    
    printf("提醒设置: %d次首次提前%d分钟\n", 
          item->reminder_count, item->reminder_interval);
    printf("状态: %s\n", get_schedule_status(item));
    
    printf("\n按任意键返回...");
    getchar();
}

/* 提醒计算核心逻辑 */
static void calculate_reminders(const char *exact_time, int interval, int count, char (*times)[6]) {
    int h, m;
    sscanf(exact_time, "%d:%d", &h, &m);
    
    int total_min = h * 60 + m - interval;
    for (int i = 0; i < count; i++) {
        snprintf(times[i], 6, "%02d:%02d", 
                (total_min + i) / 60 % 24, 
                (total_min + i) % 60);
    }
}
/*
void* check_reminders(void *arg) {
    (void)arg; // 显式声明未使用参数
    char current_date[11], current_time[6];
    
    while (running) {
        get_current_datetime(current_date, current_time);
        
        for (int i = 0; i < schedule_count; i++) {
            ScheduleItem *item = &schedules[i];
            if (item->reminder_count == 0) continue;
            
            // 日期检查
            if (strcmp(current_date, item->start_date) < 0) continue;
            if (item->end_date[0] && strcmp(current_date, item->end_date) > 0) continue;
            
            // 日规则检查
            bool day_ok = false;
            if (!item->is_repeated) {
                day_ok = (strcmp(current_date, item->end_date) == 0);
            } else {
                struct tm tm = {0};
                char *ret = strptime(current_date, "%Y-%m-%d", &tm);
                if (ret == NULL) continue;
                
                switch(item->repeat_type[0]) {
                    case 'd': day_ok = true; break;
                    case 'w': day_ok = (tm.tm_wday == (item->which_day - 1) % 7); break;
                    case 'm': day_ok = (tm.tm_mday == item->which_day); break;
                }
            }
            
            // 时间检查
            if (day_ok) {
                char reminders[item->reminder_count][6];
                calculate_reminders(item->exact_time, 
                                  item->reminder_interval,
                                  item->reminder_count,
                                  reminders);
                
                for (int j = 0; j < item->reminder_count; j++) {
                    if (strcmp(current_time, reminders[j]) == 0) {
                        clear_screen();
                        printf("\n=== 提醒 ===\n");
                        printf("日程: %s\n时间: %s\n", item->name, item->exact_time);
                        printf("\n将在30秒后自动关闭...");
                        fflush(stdout);
                        sleep(30);
                    }
                }
            }
        }
        sleep(60); // 每分钟检查一次
    }
    return NULL;
}*/


// [原有其他代码保持不变...]

void* check_reminders(void *arg) {
    (void)arg;
    char current_date[11], current_time[6];
    time_t last_alert_time = 0;
    ScheduleItem *last_alert_item = NULL;
    
    while (running) {
        get_current_datetime(current_date, current_time);
        time_t now = time(NULL);
        
        for (int i = 0; i < schedule_count; i++) {
            ScheduleItem *item = &schedules[i];
            if (item->reminder_count == 0) continue;
            
            /* 日期有效性检查 - 修改后的逻辑 */
            bool date_valid = true;
            if (item->is_repeated) {
                // 重复任务检查start_date
                if (strcmp(current_date, item->start_date) < 0) date_valid = false;
                if (item->end_date[0] && strcmp(current_date, item->end_date) > 0) date_valid = false;
            } else {
                // 非重复任务检查exact_date（新增字段）
                if (strcmp(current_date, item->exact_date) != 0) date_valid = false;
            }
            if (!date_valid) continue;
            
            /* 日规则检查 */
            bool should_alert = false;
            if (item->is_repeated) {
                struct tm tm;
                memset(&tm, 0, sizeof(struct tm));
                char *ret = strptime(current_date, "%Y-%m-%d", &tm);
                if (!ret) continue;
                
                switch(item->repeat_type[0]) {
                    case 'd': should_alert = true; break;
                    case 'w': should_alert = (tm.tm_wday == (item->which_day - 1) % 7); break;
                    case 'm': should_alert = (tm.tm_mday == item->which_day); break;
                }
            } else {
                should_alert = true; // 非重复任务只需日期匹配
            }

            /* 时间检查 */
            if (should_alert) {
                char reminders[item->reminder_count][6];
                calculate_reminders(item->exact_time, 
                                   item->reminder_interval,
                                   item->reminder_count,
                                   reminders);
                
                for (int j = 0; j < item->reminder_count; j++) {
                    if (strcmp(current_time, reminders[j]) == 0) {
                        // 防止同一日程重复提醒
                        if (last_alert_item == item && now - last_alert_time < 30) {
                            continue;
                        }
                        
                        last_alert_time = now;
                        last_alert_item = item;
                        
                        /* 显示提醒 */
                        clear_screen();
                        printf("\n=== 日程提醒 ===\n");
                        printf("  ● 名称: %s\n", item->name);
                        printf("  ● 时间: %s\n", item->exact_time);
                        printf("\n[按q键返回] 自动返回倒计时: ");
                        
                        /* 改进的倒计时处理 */
                        time_t start = time(NULL);
                        int remaining = 30;
                        while (remaining > 0 && running) {
                            printf("%2d秒", remaining);
                            fflush(stdout);
                            
                            // 非阻塞键盘检测
                            struct timeval tv = {0, 100000}; // 100ms
                            fd_set fds;
                            FD_ZERO(&fds);
                            FD_SET(STDIN_FILENO, &fds);
                            
                            if (select(STDIN_FILENO+1, &fds, NULL, NULL, &tv) > 0) {
                                if (getchar() == 'q') break;
                            }
                            
                            // 倒计时更新
                            remaining = 30 - (int)(time(NULL) - start);
                            printf("\r[按q键返回] 自动返回倒计时: ");
                        }
                        
                        // 清空输入缓冲区
                        tcflush(STDIN_FILENO, TCIFLUSH);
                        
                        // 确保返回主界面
                        if (running) {
                            printf("\n");
                            break; // 只跳出提醒循环，不退出程序
                        }
                    }
                }
            }
        }
        sleep(1); // 每秒检查一次
    }
    return NULL;
}

/* 终端控制 */
void set_unbuffered_input() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void reset_terminal() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}