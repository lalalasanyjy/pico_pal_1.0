#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "schedule_ui.h"
#include "json_parser.h"
#include <Wire.h>
#include "paj7620.h"

#define API_URL "http://111.230.236.159:3000/api/schedules"
#define CURL_CMD "curl -s --connect-timeout 3 " API_URL
#define GES_REACTION_TIME		500				// You can adjust the reaction time according to the actual circumstance.
#define GES_ENTRY_TIME			800				// When you want to recognize the Forward/Backward gestures, your gestures' reaction time must less than GES_ENTRY_TIME(0.8s). 
#define GES_QUIT_TIME			1000

void setup()
{
	uint8_t error = 0;

	Serial.begin(9600);
	Serial.println("\nPAJ7620U2 TEST DEMO: Recognize 9 gestures.");

	error = paj7620Init();			// initialize Paj7620 registers
	if (error) 
	{
		Serial.print("INIT ERROR,CODE:");
		Serial.println(error);
	}
	else
	{
		Serial.println("INIT OK");
	}
	Serial.println("Please input your gestures:\n");
}

// 默认的本地JSON数据
const char *default_json_data = 
"{"
"  \"schedule_count\": 4,"
"  \"schedules\": ["
"    {"
"      \"id\": 1,"
"      \"name\": \"每周会议\","
"      \"is_repeated\": true,"
"      \"repeat_type\": \"week\","
"      \"which_day\": 7,"
"      \"start_date\": \"2023-11-01\","
"      \"end_date\": null,"
"      \"exact_date\": null,"
"      \"reminder_count\": 1,"
"      \"reminder_interval\": 5,"
"      \"exact_time\": \"16:11\""
"    },"
"    {"
"      \"id\": 2,"
"      \"name\": \"项目评审\","
"      \"is_repeated\": false,"
"      \"repeat_type\": null,"
"      \"which_day\": null,"
"      \"start_date\": null,"
"      \"end_date\": null,"
"      \"exact_date\": \"2026-12-15\","
"      \"reminder_count\": 2,"
"      \"reminder_interval\": 10,"
"      \"exact_time\": \"16:19\""
"    },"
"    {"
"      \"id\": 3,"
"      \"name\": \"每日站会\","
"      \"is_repeated\": true,"
"      \"repeat_type\": \"day\","
"      \"which_day\": null,"
"      \"start_date\": \"2023-11-01\","
"      \"end_date\": null,"
"      \"exact_date\": null,"
"      \"reminder_count\": 1,"
"      \"reminder_interval\": 5,"
"      \"exact_time\": \"16:16\""
"    },"
"    {"
"      \"id\": 4,"
"      \"name\": \"月度报告\","
"      \"is_repeated\": true,"
"      \"repeat_type\": \"month\","
"      \"which_day\": 1,"
"      \"start_date\": \"2023-11-01\","
"      \"end_date\": \"2024-11-01\","
"      \"exact_date\": null,"
"      \"reminder_count\": 1,"
"      \"reminder_interval\": 5,"
"      \"exact_time\": \"16:17\""
"    }"
"  ]"
"}";

// 从网络获取JSON数据
char *fetch_online_json() {
    FILE *fp;
    char buffer[4096];
    char *json_data = NULL;
    size_t json_size = 0;

    // 调用curl命令
    fp = popen(CURL_CMD, "r");
    if (fp == NULL) {
        perror("Failed to run curl");
        return NULL;
    }

    // 读取curl输出
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        size_t chunk_size = strlen(buffer);
        char *new_data = realloc(json_data, json_size + chunk_size + 1);
        if (!new_data) {
            perror("Memory allocation failed");
            free(json_data);
            pclose(fp);
            return NULL;
        }
        json_data = new_data;
        strcpy(json_data + json_size, buffer);
        json_size += chunk_size;
    }

    // 关闭管道
    int status = pclose(fp);
    if (status != 0) {
        fprintf(stderr, "curl failed with exit code %d\n", status);
        free(json_data);
        return NULL;
    }

    // 验证获取的数据是否是有效的JSON
    if (json_data && json_data[0] != '{' && json_data[0] != '[') {
        fprintf(stderr, "Invalid JSON data received\n");
        free(json_data);
        return NULL;
    }

    return json_data;
}
/*
int main() {

    char *json_data = NULL;
    setup();
    // 尝试从网络获取数据
    json_data = fetch_online_json();
    
    // 如果网络获取失败，使用默认数据
    if (json_data == NULL) {
        fprintf(stderr, "Using default local data\n");
        json_data = strdup(default_json_data);
    }

    if (!parse_json(json_data)) {
        fprintf(stderr, "JSON解析失败\n");
        free(json_data);
        return 1;
    }
    free(json_data);

    set_unbuffered_input();
    
    // 启动提醒线程
    pthread_t reminder_thread;
    pthread_create(&reminder_thread, NULL, check_reminders, NULL);
    
    // 主界面循环
    char ch;
    while (running) {
        show_list();
        ch = getchar();
        
        switch (ch) {
            case 27: // 方向键
                if (getchar() == 91) {
                    switch (getchar()) {
                        case 'A': if (selected_index > 0) selected_index--; break;
                        case 'B': if (selected_index < schedule_count - 1) selected_index++; break;
                        case 'C': show_detail(selected_index); break;
                        case 'D': selected_index = 0; break;
                    }
                }
                break;
            case '\n': show_detail(selected_index); break;
            case 'q': running = false; break;
        }
    }
    
    pthread_join(reminder_thread, NULL);
    reset_terminal();
    return 0;
}
    */
   int main() {
    char *json_data = NULL;
    uint8_t data = 0, data1 = 0;
    int error;

    setup();
    // 尝试从网络获取数据
    json_data = fetch_online_json();
    
    // 如果网络获取失败，使用默认数据
    if (json_data == NULL) {
        fprintf(stderr, "Using default local data\n");
        json_data = strdup(default_json_data);
    }

    if (!parse_json(json_data)) {
        fprintf(stderr, "JSON解析失败\n");
        free(json_data);
        return 1;
    }
    free(json_data);

    set_unbuffered_input();
    
    // 启动提醒线程
    pthread_t reminder_thread;
    pthread_create(&reminder_thread, NULL, check_reminders, NULL);
    
    // 主界面循环
    while (running) {
        show_list();
        
        // 替换原来的getchar()为手势检测
        error = paj7620ReadReg(0x43, 1, &data);
        if (!error) {
            switch (data) {
                case GES_UP_FLAG:
                    delay(GES_ENTRY_TIME);
                    paj7620ReadReg(0x43, 1, &data);
                    if(data != GES_FORWARD_FLAG && data != GES_BACKWARD_FLAG) {
                        // 替换原来的case A逻辑
                        if (selected_index > 0) selected_index--;
                    }
                    break;
                case GES_DOWN_FLAG:
                    delay(GES_ENTRY_TIME);
                    paj7620ReadReg(0x43, 1, &data);
                    if(data != GES_FORWARD_FLAG && data != GES_BACKWARD_FLAG) {
                        // 替换原来的case B逻辑
                        if (selected_index < schedule_count - 1) selected_index++;
                    }
                    break;
                case GES_RIGHT_FLAG:
                    delay(GES_ENTRY_TIME);
                    paj7620ReadReg(0x43, 1, &data);
                    if(data != GES_FORWARD_FLAG && data != GES_BACKWARD_FLAG) {
                        // 替换原来的case C逻辑
                        show_detail(selected_index);
                    }
                    break;
                default:
                    paj7620ReadReg(0x44, 1, &data1);
                    if (data1 == GES_WAVE_FLAG) {
                        // 替换原来的case D逻辑
                        selected_index = 0;
                    }
                    break;
            }
        }
        
        // 保留原来的退出和回车功能
        if (kbhit()) {
            char ch = getchar();
            if (ch == '\n') {
                show_detail(selected_index);
            } else if (ch == 'q') {
                running = false;
            }
        }
        
        delay(100);
    }
    
    pthread_join(reminder_thread, NULL);
    reset_terminal();
    return 0;
}