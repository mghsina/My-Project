#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "ripple_solver.h"

// 初始化谜题
bool init_puzzle(puzzle_state_t *state, int N) {
    state->N = N;
    state->running = true;
    state->auto_solve_request = false;
    state->move_count = 0;
    state->hint_count = 0;
    
    // 分配内存
    state->board = malloc(N * sizeof(int *));
    state->rooms = malloc(N * sizeof(int *));
    state->original = malloc(N * sizeof(int *));
    
    if (!state->board || !state->rooms || !state->original) {
        return false;
    }
    
    for (int i = 0; i < N; i++) {
        state->board[i] = calloc(N, sizeof(int));
        state->rooms[i] = calloc(N, sizeof(int));
        state->original[i] = calloc(N, sizeof(int));
        if (!state->board[i] || !state->rooms[i] || !state->original[i]) {
            return false;
        }
    }
    
    // 初始化移动历史
    state->history = malloc(MAX_MOVES * sizeof(move_t));
    if (!state->history) return false;
    state->history_count = 0;
    
    // 读取谜题文件
    char filename[50];
    sprintf(filename, "puzzle%c.txt", (N == 6) ? 'A' : 'B');
    
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Cannot open file: %s \n", filename);
        return false;
    }
    
    int file_N;
    fscanf(fp, "%d", &file_N);
    
    // 读取初始盘面
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fscanf(fp, "%d", &state->board[i][j]);
            state->original[i][j] = state->board[i][j];
        }
    }
    
    // 读取房间定义
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fscanf(fp, "%d", &state->rooms[i][j]);
        }
    }
    
    fclose(fp);
    
    // 计算每个房间的大小
    memset(state->room_sizes, 0, sizeof(state->room_sizes));
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int rid = state->rooms[i][j];
            state->room_sizes[rid]++;
        }
    }
    
    return true;
}

// 获取房间大小
int get_room_size(puzzle_state_t *state, int row, int col) {
    int rid = state->rooms[row][col];
    return state->room_sizes[rid];
}

// 检查房间约束
bool check_room_constraint(puzzle_state_t *state, int row, int col, int num) {
    int rid = state->rooms[row][col];
    
    // 检查房间内是否已有相同数字
    for (int i = 0; i < state->N; i++) {
        for (int j = 0; j < state->N; j++) {
            if (state->rooms[i][j] == rid && state->board[i][j] == num) {
                return false;
            }
        }
    }
    
    return true;
}

// 检查距离约束
bool check_distance_constraint(puzzle_state_t *state, int row, int col, int num) {
    // 检查同一行
    for (int j = 0; j < state->N; j++) {
        if (j == col) continue;
        if (state->board[row][j] == num) {
            int distance = abs(j - col) - 1;
            if (distance < num) {
                return false;
            }
        }
    }
    
    // 检查同一列
    for (int i = 0; i < state->N; i++) {
        if (i == row) continue;
        if (state->board[i][col] == num) {
            int distance = abs(i - row) - 1;
            if (distance < num) {
                return false;
            }
        }
    }
    
    return true;
}

// 验证移动是否有效
bool is_valid_move(puzzle_state_t *state, int row, int col, int num) {
    if (row < 0 || row >= state->N || col < 0 || col >= state->N) {
        return false;
    }
    
    // 不能修改原始数字
    if (state->original[row][col] != 0) {
        return false;
    }
    
    // 检查数字范围
    int max_num = get_room_size(state, row, col);
    if (num < 1 || num > max_num) {
        return false;
    }
    
    // 检查房间约束
    if (!check_room_constraint(state, row, col, num)) {
        return false;
    }
    
    // 检查距离约束
    if (!check_distance_constraint(state, row, col, num)) {
        return false;
    }
    
    return true;
}

// 放置数字
bool place_number(puzzle_state_t *state, int row, int col, int num) {
    if (!is_valid_move(state, row, col, num)) {
        return false;
    }
    
    // 保存到历史
    if (state->history_count < MAX_MOVES) {
        state->history[state->history_count].row = row;
        state->history[state->history_count].col = col;
        state->history[state->history_count].old_value = state->board[row][col];
        state->history[state->history_count].new_value = num;
        state->history_count++;
    }
    
    state->board[row][col] = num;
    state->move_count++;
    
    return true;
}

// 撤销移动
bool undo_move(puzzle_state_t *state) {
    if (state->history_count == 0) {
        printf("No cancelled move.\n");
        return false;
    }
    
    state->history_count--;
    move_t *last = &state->history[state->history_count];
    state->board[last->row][last->col] = last->old_value;
    state->move_count--;
    
    printf("You have cancelled the move on (%d,%d)\n", last->row + 1, last->col + 1);
    return true;
}

// 重置谜题
void reset_puzzle(puzzle_state_t *state) {
    for (int i = 0; i < state->N; i++) {
        for (int j = 0; j < state->N; j++) {
            state->board[i][j] = state->original[i][j];
        }
    }
    state->history_count = 0;
    state->move_count = 0;
    printf("You have reset the puzzle.\n");
}

// 回溯求解器
bool backtrack_solve(puzzle_state_t *state, int row, int col) {
    if (row >= state->N) {
        return true;
    }
    
    int next_row = row;
    int next_col = col + 1;
    if (next_col >= state->N) {
        next_row++;
        next_col = 0;
    }
    
    // 跳过预填数字
    if (state->original[row][col] != 0) {
        return backtrack_solve(state, next_row, next_col);
    }
    
    int max_num = get_room_size(state, row, col);
    
    for (int num = 1; num <= max_num; num++) {
        if (is_valid_move(state, row, col, num)) {
            state->board[row][col] = num;
            
            if (backtrack_solve(state, next_row, next_col)) {
                return true;
            }
            
            state->board[row][col] = 0;
        }
    }
    
    return false;
}

// 异步求解
void solve_puzzle_async(puzzle_state_t *state) {
    printf("开始求解...\n");
    
    if (backtrack_solve(state, 0, 0)) {
        printf("求解完成！\n");
    } else {
        printf("求解失败！谜题可能无解。\n");
    }
}

// 检查谜题是否完成
bool is_puzzle_complete(puzzle_state_t *state) {
    for (int i = 0; i < state->N; i++) {
        for (int j = 0; j < state->N; j++) {
            if (state->board[i][j] == 0) {
                return false;
            }
            
            // 验证最终盘面
            if (!check_distance_constraint(state, i, j, state->board[i][j])) {
                return false;
            }
        }
    }
    return true;
}

// 提供提示
void provide_hint(puzzle_state_t *state) {
    // 保存当前状态
    int **saved_board = malloc(state->N * sizeof(int *));
    for (int i = 0; i < state->N; i++) {
        saved_board[i] = malloc(state->N * sizeof(int));
        memcpy(saved_board[i], state->board[i], state->N * sizeof(int));
    }
    
    // 尝试求解并找到下一个数字
    if (backtrack_solve(state, 0, 0)) {
        // 找到第一个不同的位置
        for (int i = 0; i < state->N; i++) {
            for (int j = 0; j < state->N; j++) {
                if (saved_board[i][j] != state->board[i][j] && 
                    state->original[i][j] == 0) {
                    printf("\n💡 Hint: you can place number %d on the position (%d,%d)\n", state->board[i][j], i + 1, j + 1);
                    state->hint_count++;
                    break;
                }
            }
        }
    } else {
        printf("Cannot provide hint.Please check the board.\n");
    }
    
    // 恢复状态
    for (int i = 0; i < state->N; i++) {
        memcpy(state->board[i], saved_board[i], state->N * sizeof(int));
        free(saved_board[i]);
    }
    free(saved_board);
}

// 清理资源
void cleanup_puzzle(puzzle_state_t *state) {
    for (int i = 0; i < state->N; i++) {
        free(state->board[i]);
        free(state->rooms[i]);
        free(state->original[i]);
    }
    free(state->board);
    free(state->rooms);
    free(state->original);
    free(state->history);
}