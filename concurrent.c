#include"concurrent.h"
#include"ripple_solver.h"
#include"ui.h"
#include<stdio.h>
#include<pthread.h>
static pthread_t timer_thread;
static pthread_t save_thread;
static volatile int timer_running = 0;
static time_t start_time;
static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t save_mutex = PTHREAD_MUTEX_INITIALIZER;

// 计时器线程
void *timer_function(void *arg) {
    puzzle_state_t *state = (puzzle_state_t *)arg;

    while (timer_running && state->running) {
        sleep(1);
        pthread_mutex_lock(&timer_mutex);
        state->elapsed_time = time(NULL) - start_time;
        pthread_mutex_unlock(&timer_mutex);
    }

    return NULL;
}

// 启动计时器
void start_timer(puzzle_state_t *state) {
    timer_running = 1;
    start_time = time(NULL);
    state->elapsed_time = 0;
    pthread_create(&timer_thread, NULL, timer_function, state);
}

// 停止计时器
void stop_timer(puzzle_state_t *state) {
    timer_running = 0;
    pthread_join(timer_thread, NULL);
}

// 显示计时器
void display_timer(puzzle_state_t *state) {
    int hours = state->elapsed_time / 3600;
    int minutes = (state->elapsed_time % 3600) / 60;
    int seconds = state->elapsed_time % 60;

    printf("\nTime: %02d:%02d:%02d\n", hours, minutes, seconds);
}

// 自动保存线程
void *auto_save_function(void *arg) {
    puzzle_state_t *state = (puzzle_state_t *)arg;
    char filename[50];

    while (state->running) {
        sleep(30); // 每30秒自动保存

        pthread_mutex_lock(&save_mutex);

        sprintf(filename, "save_%d.txt", (int)time(NULL));
        FILE *fp = fopen(filename, "w");
        if (fp) {
            fprintf(fp, "%d\n", state->N);
            for (int i = 0; i < state->N; i++) {
                for (int j = 0; j < state->N; j++) {
                    fprintf(fp, "%d ", state->board[i][j]);
                }
                fprintf(fp, "\n");
            }
            fclose(fp);
            printf("\nSaved to %s\n", filename);
        }

        pthread_mutex_unlock(&save_mutex);
    }

    return NULL;
}

// 启动自动保存
void start_auto_save(puzzle_state_t *state) {
    pthread_create(&save_thread, NULL, auto_save_function, state);
}

// 显示统计信息
void display_stats(puzzle_state_t *state) {
    printf("\n=== 游戏统计 ===\n");
    printf("总移动次数: %d\n", state->move_count);
    printf("请求提示次数: %d\n", state->hint_count);
    display_timer(state);

    // 计算完成度
    int filled = 0;
    int total = state->N * state->N;
    for (int i = 0; i < state->N; i++) {
        for (int j = 0; j < state->N; j++) {
            if (state->board[i][j] != 0) filled++;
        }
    }
    printf("完成度: %.1f%%\n", (float)filled / total * 100);
}

// 检查进度
void check_progress(puzzle_state_t *state) {
    int filled = 0;
    int errors = 0;

    for (int i = 0; i < state->N; i++) {
        for (int j = 0; j < state->N; j++) {
            if (state->board[i][j] != 0) {
                filled++;
                if (!check_distance_constraint(state, i, j, state->board[i][j])) {
                    errors++;
                }
            }
        }
    }

    printf("\n=== 进度检查 ===\n");
    printf("已填充: %d/%d\n", filled, state->N * state->N);
    printf("错误: %d\n", errors);

    if (errors > 0) {
        printf("⚠️  发现错误，请检查！\n");
    }
}

// 演示模式
void start_demo_mode(puzzle_state_t *state) {
    printf("\n🎬 启动演示模式...\n");
    solve_puzzle_async(state);
    display_board(state);
}