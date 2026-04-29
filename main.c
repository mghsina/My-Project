#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include "ripple_solver.h"
#include "ui.h"
#include "concurrent.h"

volatile sig_atomic_t solver_running = 1;
volatile sig_atomic_t show_hint_request = 0;
puzzle_state_t *global_state = NULL;

static pthread_t timer_thread;
static pthread_t save_thread;
static volatile int timer_running = 0;
static time_t start_time;
static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t save_mutex = PTHREAD_MUTEX_INITIALIZER;

static char last_error[256];

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

void start_timer(puzzle_state_t *state) {
    timer_running = 1;
    start_time = time(NULL);
    state->elapsed_time = 0;
    pthread_create(&timer_thread, NULL, timer_function, state);
}

void stop_timer(puzzle_state_t *state) {
    timer_running = 0;
    pthread_join(timer_thread, NULL);
}

void display_timer(puzzle_state_t *state) {
    int hours = state->elapsed_time / 3600;
    int minutes = (state->elapsed_time % 3600) / 60;
    int seconds = state->elapsed_time % 60;

    printf("\nTime: %02d:%02d:%02d\n", hours, minutes, seconds);
}

void *auto_save_function(void *arg) {
    puzzle_state_t *state = (puzzle_state_t *)arg;
    char filename[50];

    while (state->running) {
        sleep(30);

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
        }

        pthread_mutex_unlock(&save_mutex);
    }

    return NULL;
}

void start_auto_save(puzzle_state_t *state) {
    pthread_create(&save_thread, NULL, auto_save_function, state);
}

void display_stats(puzzle_state_t *state) {
    printf("\n=== Game statistics ===\n");
    printf("#Moves: %d\n", state->move_count);
    printf("#Hints: %d\n", state->hint_count);
    display_timer(state);

    int filled = 0;
    int total = state->N * state->N;
    for (int i = 0; i < state->N; i++) {
        for (int j = 0; j < state->N; j++) {
            if (state->board[i][j] != 0) filled++;
        }
    }
    printf("Completeness: %.1f%%\n", (float)filled / total * 100);
}

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

    printf("\n=== Process check ===\n");
    printf("You have filled in: %d/%d\n", filled, state->N * state->N);
    printf("Errors: %d\n", errors);

    if (errors > 0) {
        printf("⚠️Error found,please check！\n");
    }
}

void start_demo_mode(puzzle_state_t *state) {
    printf("\n🎬Start demo mode...\n");
    solve_puzzle_async(state);
}

void display_board(puzzle_state_t *state) {
    int N = state->N;

    printf("\n");

    printf("   ");
    for (int j = 0; j < N; j++) {
        printf(" %2d ", j + 1);
    }
    printf("\n");

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
        printf("%2d│", i + 1);

        for (int j = 0; j < N; j++) {
            if (state->board[i][j] == 0) {
                printf(" · ");
            } else {
                if (state->original[i][j] != 0) {
                    printf(" %d ", state->board[i][j]);
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

    printf("\n⏱️  Time: ");
    int hours = state->elapsed_time / 3600;
    int minutes = (state->elapsed_time % 3600) / 60;
    int seconds = state->elapsed_time % 60;
    printf("%02d:%02d:%02d  ", hours, minutes, seconds);
    printf("Move: %d  ", state->move_count);
    printf("Hint: %d\n", state->hint_count);
}
void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\n[!] Received safety signal.Trying to exit...\n");
        solver_running = 0;
    } else if (sig == SIGUSR1) {
        printf("\n[?] request help...\n");
        show_hint_request = 1;
    } else if (sig == SIGUSR2) {
        printf("\n[!] Request for ripple solver's help...\n");
        if (global_state) {
            global_state->auto_solve_request = 1;
        }
    }
}

void setup_signal_handlers() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
}

void show_usage() {
    printf("\n=== Ripple Effect solver ===\n");
    printf("Order:\n");
    printf("  q/quit     - exit program\n");
    printf("  h/hint     - get hint\n");
    printf("  s/solve    - automatic solving\n");
    printf("  u/undo     - cancel last step\n");
    printf("  r/reset    - reset puzzle\n");
    printf("  d/demo     - demostration mode\n");
    printf("  t/time     - represent time\n");
    printf("  c/check    - check present process\n");
    printf("\n");

}

void *command_handler(void *arg) {
    puzzle_state_t *state = (puzzle_state_t *)arg;
    char input[100];

    while (state->running && solver_running) {
        printf("\nInput (row col num) or command (q/h/s/u/r/d/t/c): ");
        if (fgets(input, sizeof(input), stdin)) {
            input[strcspn(input, "\n")] = 0;
            int row, col, num;
            if (sscanf(input, "%d %d %d", &row, &col, &num) == 3) {
                row--; col--;
                if (place_number(state, row, col, num)) {
                    display_board(state);
                }
            }

            else if (strcmp(input, "q") == 0 || strcmp(input, "quit") == 0) {
                state->running = 0;
                solver_running = 0;
                break;
            } else if (strcmp(input, "h") == 0 || strcmp(input, "hint") == 0) {
                show_hint_request = 1;
            } else if (strcmp(input, "s") == 0 || strcmp(input, "solve") == 0) {
                state->auto_solve_request = 1;
            } else if (strcmp(input, "u") == 0 || strcmp(input, "undo") == 0) {
                undo_move(state);
                display_board(state);
            } else if (strcmp(input, "r") == 0 || strcmp(input, "reset") == 0) {
                reset_puzzle(state);
                display_board(state);
            } else if (strcmp(input, "d") == 0 || strcmp(input, "demo") == 0) {
                start_demo_mode(state);
            } else if (strcmp(input, "t") == 0 || strcmp(input, "time") == 0) {
                display_timer(state);
            } else if (strcmp(input, "c") == 0 || strcmp(input, "check") == 0) {
                check_progress(state);
            }else if (strlen(input) > 0 && strcmp(input, "") != 0 ) {
                printf("❌ Unknown command: '%s'\n", input);
                show_usage();
            }
        }
    }
    return NULL;
}

bool init_puzzle(puzzle_state_t *state, int N) {
    state->N = N;
    state->running = true;
    state->auto_solve_request = false;
    state->move_count = 0;
    state->hint_count = 0;

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

    state->history = malloc(MAX_MOVES * sizeof(move_t));
    if (!state->history) return false;
    state->history_count = 0;

    const char* filename = (N == 6) ? "puzzleA.txt" : "puzzleB.txt";

    FILE *fp = fopen(filename, "r");
    if (fp==NULL) {
        printf("Cannot open file: %s \n", filename);
        return false;
    }

    int file_N;
    fscanf(fp, "%d", &file_N);

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fscanf(fp, "%d", &state->board[i][j]);
            state->original[i][j] = state->board[i][j];
        }
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fscanf(fp, "%d", &state->rooms[i][j]);
        }
    }

    fclose(fp);

    memset(state->room_sizes, 0, sizeof(state->room_sizes));
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int rid = state->rooms[i][j];
            state->room_sizes[rid]++;
        }
    }

    return true;
}

int get_room_size(puzzle_state_t *state, int row, int col) {
    int rid = state->rooms[row][col];
    return state->room_sizes[rid];
}

bool check_room_constraint(puzzle_state_t *state, int row, int col, int num) {
    int rid = state->rooms[row][col];

    for (int i = 0; i < state->N; i++) {
        for (int j = 0; j < state->N; j++) {
            if (state->rooms[i][j] == rid && state->board[i][j] == num) {
                return false;
            }
        }
    }

    return true;
}

bool check_distance_constraint(puzzle_state_t *state, int row, int col, int num) {
    for (int j = 0; j < state->N; j++) {
        if (j == col) continue;
        if (state->board[row][j] == num) {
            int distance = abs(j - col) - 1;
            if (distance < num) {
                return false;
            }
        }
    }

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

bool is_valid_move(puzzle_state_t *state, int row, int col, int num) {
    if (row < 0 || row >= state->N || col < 0 || col >= state->N) {
        sprintf(last_error, "Position out of range (%d,%d)", row + 1, col + 1);
        return false;
    }

    if (state->original[row][col] != 0) {
        sprintf(last_error, "Cannot modify original number at (%d,%d)", row + 1, col + 1);
        return false;
    }

    int max_num = get_room_size(state, row, col);
    if (num < 1 || num > max_num) {
        sprintf(last_error, "Number %d out of range for this room (1-%d)", num, max_num);
        return false;
    }

    if (!check_room_constraint(state, row, col, num)) {
        sprintf(last_error, "Number %d already exists in this room", num);
        return false;
    }


    if (!check_distance_constraint(state, row, col, num)) {
        sprintf(last_error, "Number %d violates distance constraint (same row/col)", num);
        return false;
    }

    return true;
}


bool place_number(puzzle_state_t *state, int row, int col, int num) {
    if (!is_valid_move(state, row, col, num)) {
        printf("❌ Invalid move: %s\n", last_error);
        return false;
    }


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


    if (state->original[row][col] != 0) {
        return backtrack_solve(state, next_row, next_col);
    }
    if (state->board[row][col] != 0) {
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


void solve_puzzle_async(puzzle_state_t *state) {
    printf("Start solving...\n");

    if (backtrack_solve(state, 0, 0)) {
        printf("Solution completed！\n");
        display_board(state);
        display_stats(state);
        exit(0);
    } else {
        printf("Solution failed!May have no solution.\n");
    }
}


bool is_puzzle_complete(puzzle_state_t *state) {
    for (int i = 0; i < state->N; i++) {
        for (int j = 0; j < state->N; j++) {
            if (state->board[i][j] == 0) {
                return false;
            }


            if (!check_distance_constraint(state, i, j, state->board[i][j])) {
                return false;
            }
            if (!check_room_constraint(state,i,j,state->board[i][j])) {
                return false;
            }
            if (!is_valid_move(state,i,j,state->board[i][j])) {
                return false;
            }
        }
    }
    return true;
}


void provide_hint(puzzle_state_t *state) {

    int **saved_board = malloc(state->N * sizeof(int *));
    for (int i = 0; i < state->N; i++) {
        saved_board[i] = malloc(state->N * sizeof(int));
        memcpy(saved_board[i], state->board[i], state->N * sizeof(int));
    }


    if (backtrack_solve(state, 0, 0)) {

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

int main(int argc, char *argv[]) {
    int N;
    puzzle_state_t state;

    printf("=== Ripple Effect solver ===\n");


    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        show_usage();
        return 0;
    }


    if (argc > 1) {
        N = atoi(argv[1]);
        if (N != 6 && N != 8) {
            printf("Error:Only 6*6 or 8*8 supported\n");
            return 1;
        }
    } else {
        printf("Choose puzzle size (6 or 8): ");
        scanf("%d", &N);
        while (getchar() != '\n');
    }


    if (!init_puzzle(&state, N)) {
        printf("Failed to initialize!\n");
        return 1;
    }

    global_state = &state;


    setup_signal_handlers();


    display_board(&state);
    show_usage();


    start_timer(&state);


    pthread_t cmd_thread;
    pthread_create(&cmd_thread, NULL, command_handler, &state);


    start_auto_save(&state);


    while (state.running && solver_running) {

        if (show_hint_request) {
            provide_hint(&state);
            show_hint_request = 0;
            display_board(&state);
            printf("\nInput (row col num) or command (q/h/s/u/r/d/t/c): ");
            fflush(stdout);
        }


        if (state.auto_solve_request) {
            printf("\n[!] Start automatic solution...\n");
            solve_puzzle_async(&state);
            state.auto_solve_request = 0;
            display_board(&state);
            printf("\nInput (row col num) or command (q/h/s/u/r/d/t/c): ");
            fflush(stdout);
        }


        if (is_puzzle_complete(&state)) {
            printf("\n🎉 Congratulations! 🎉\n");
            stop_timer(&state);
            display_stats(&state);
            break;
        }



        usleep(100000);
    }


    pthread_join(cmd_thread, NULL);
    cleanup_puzzle(&state);

    printf("\nThanks for your playing！\n");
    return 0;
}