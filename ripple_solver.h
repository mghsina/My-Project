#ifndef RIPPLE_SOLVER_H
#define RIPPLE_SOLVER_H

#include <stdbool.h>
#include <stdio.h>
#define MAX_MOVES 1000

typedef struct {
    int row;
    int col;
    int old_value;
    int new_value;
} move_t;

typedef struct {
    int N;              //board size
    int **board;        // 当前盘面
    int **rooms;        // 房间定义
    int **original;     // 原始盘面
    int room_sizes[100]; // 房间大小
    move_t *history;     // 移动历史
    int history_count;
    int move_count;
    int hint_count;
    int elapsed_time;
    volatile bool running;
    volatile bool auto_solve_request;
} puzzle_state_t;

// 初始化函数
bool init_puzzle(puzzle_state_t *state, int N);
void cleanup_puzzle(puzzle_state_t *state);

// 游戏逻辑
bool is_valid_move(puzzle_state_t *state, int row, int col, int num);
bool place_number(puzzle_state_t *state, int row, int col, int num);
bool undo_move(puzzle_state_t *state);
void reset_puzzle(puzzle_state_t *state);

// 求解器
void solve_puzzle_async(puzzle_state_t *state);
bool backtrack_solve(puzzle_state_t *state, int row, int col);
bool is_puzzle_complete(puzzle_state_t *state);

// 辅助函数
int get_room_size(puzzle_state_t *state, int row, int col);
bool check_room_constraint(puzzle_state_t *state, int row, int col, int num);
bool check_distance_constraint(puzzle_state_t *state, int row, int col, int num);
void provide_hint(puzzle_state_t *state);

#endif