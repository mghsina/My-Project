#ifndef CONCURRENT_H
#define CONCURRENT_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "concurrent.h"
#include "ripple_solver.h"



// 计时器线程
void *timer_function(void *arg);
// 启动计时器
void start_timer(puzzle_state_t *state);
// 停止计时器
void stop_timer(puzzle_state_t *state);
// 显示计时器
void display_timer(puzzle_state_t *state);

// 自动保存线程
void *auto_save_function(void *arg);

// 启动自动保存
void start_auto_save(puzzle_state_t *state);

// 显示统计信息
void display_stats(puzzle_state_t *state);

// 检查进度
void check_progress(puzzle_state_t *state);

// 演示模式
void start_demo_mode(puzzle_state_t *state);
#endif