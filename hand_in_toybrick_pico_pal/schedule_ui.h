#ifndef SCHEDULE_UI_H
#define SCHEDULE_UI_H

#include "json_parser.h"

extern int selected_index;
extern bool running;

// 界面功能
void clear_screen();
void show_list();
void show_detail(int index);

// 终端控制
void set_unbuffered_input();
void reset_terminal();

// 提醒功能
void* check_reminders(void *arg);
void show_alert(const char *name, const char *time);

#endif