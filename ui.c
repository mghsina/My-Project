#include"ui.h"
void display_board(puzzle_state_t *state) {
    int N = state->N;

    printf("\n");

    // 显示列号
    printf("   ");
    for (int j = 0; j < N; j++) {
        printf(" %2d ", j + 1);
    }
    printf("\n");

    // 显示上边框
    printf("  ┌");
    for (int i = 0; i < N; i++) {
        printf("───");
        if (i < N - 1) {
            if (state->rooms[0][i] != state->rooms[0][i + 1]) {
                printf("┬");
            } else {
                printf("─");
            }
        }
    }
    printf("┐\n");

    for (int i = 0; i < N; i++) {
        // 显示行号
        printf("%2d │", i + 1);

        // 显示数字
        for (int j = 0; j < N; j++) {
            if (state->board[i][j] == 0) {
                printf(" · ");
            } else {
                // 原始数字用不同颜色标记（这里用括号）
                if (state->original[i][j] != 0) {
                    printf("[%d]", state->board[i][j]);
                } else {
                    printf(" %d ", state->board[i][j]);
                }
            }

            if (j < N - 1) {
                if (state->rooms[i][j] != state->rooms[i][j + 1]) {
                    printf("│");
                } else {
                    printf(" ");
                }
            }
        }
        printf("│\n");

        // 显示行间分隔线
        if (i < N - 1) {
            printf("  ├");
            for (int j = 0; j < N; j++) {
                if (state->rooms[i][j] != state->rooms[i + 1][j]) {
                    printf("───");
                } else {
                    printf("   ");
                }
                if (j < N - 1) {
                    if (state->rooms[i][j] != state->rooms[i][j + 1] ||
                        state->rooms[i + 1][j] != state->rooms[i + 1][j + 1]) {
                        printf("┼");
                    } else {
                        printf(" ");
                    }
                }
            }
            printf("┤\n");
        }
    }

    // 显示下边框
    printf("  └");
    for (int i = 0; i < N; i++) {
        printf("───");
        if (i < N - 1) {
            if (state->rooms[N-1][i] != state->rooms[N-1][i + 1]) {
                printf("┴");
            } else {
                printf("─");
            }
        }
    }
    printf("┘\n");

    // 显示计时
    printf("\n⏱️  Time: ");
    int hours = state->elapsed_time / 3600;
    int minutes = (state->elapsed_time % 3600) / 60;
    int seconds = state->elapsed_time % 60;
    printf("%02d:%02d:%02d  ", hours, minutes, seconds);
    printf("Move: %d  ", state->move_count);
    printf("Hint: %d\n", state->hint_count);
}