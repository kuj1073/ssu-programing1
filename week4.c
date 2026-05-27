#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termio.h>
#include <unistd.h>
#define SIZE 31
#define MAX_LEVEL 5
#define GAME 0
#define HELP 1
#define RANK 2

int getch(void);
void show_initial_screen(int);
void show_option_screen();
void show_name_screen();
void show_complete_screen();
void flush_stdin_line();
bool validate_map(char[SIZE][SIZE]);
void show_playing_map(char[], int, int, int, char[SIZE][SIZE]);
void show_help();
void copy_map(char[SIZE][SIZE], char[SIZE][SIZE], int, int);
void print_command_name(int op);
void show_moves_n_command(int, int);
void save_ranking(char[], int, int);
void save_game(char[], int, int, int, int, char[SIZE][SIZE]);
void show_ranking(int);
int get_last_level_and_load_map(char[MAX_LEVEL][SIZE][SIZE]);
void input_name(char []);

int main(void) {
  int last_level;
  char maps[MAX_LEVEL][SIZE][SIZE] = {0};
  last_level = get_last_level_and_load_map(maps);
  if (last_level == -1) {
    printf("map.txt 파일이 없습니다.\n");
    return 0;
  }
  else if(last_level == -2){
    printf("map.txt 파일에 유효하지 않은 문자가 있습니다.\n");
    return 0;
  }
  // >>> validate map >>>
  for (int i = 0; i < last_level; i++) {
    if (!validate_map(maps[i])) {
      printf("Wrong level %d map\n", i + 1);
      return 0;
    }
  }
  // <<< validate map <<<
  // >>> '.' to ' ' >>>
  for (int i = 0; i < last_level; i++)
    for (int j = 0; j < SIZE; j++)
      for (int k = 0; k < SIZE; k++)
        if (maps[i][j][k] == '.') maps[i][j][k] = ' ';
  // <<< '.' to ' ' <<<
    char option;
    char name[4] = {0};
    int playing_level = 0;
    bool load_on_start = false;
    show_initial_screen(last_level);
  while (1) {
    show_option_screen();
    char option_str[10];
    scanf("%s", option_str);
    flush_stdin_line();
    if(option_str[1] != '\0') {  // 두 글자 이상이면 무효
        printf("잘못된 입력입니다.\n");
        continue;
    }
    option = tolower(option_str[0]);
    if (option == 'f') {
        load_on_start = true;
        break;
    } else if (option == 'n') {
        input_name(name);
        playing_level = 0;
        break;
    } else if (1 <= option - '0' && option - '0' <= last_level) {
        input_name(name);
        playing_level = option - '1';
        break;
    } 
    printf("잘못된 입력입니다. 다시 입력해주세요.\n");
    }
  // >>> play에서 필요한 변수들 (SET_PLAYING_MAP_BY_PLAYING_LEVEL 에서 사용하는
  // 변수들) >>>
  char playing_map[SIZE][SIZE];
  bool box_dest_map[SIZE][SIZE];
  int fitted_map_width, fitted_map_height;
  int left_box_cnt;
  int player_y, player_x;
  bool is_complete_level;
  int moves_cnt = 0;
  int op;
  // <<< play에서 필요한 변수들 <<<

  // >>> Undo 기능을 위한 변수 선언 >>>
  char history_map[5][SIZE][SIZE];
  int history_player_y[5];
  int history_player_x[5];
  int history_left_box_cnt[5];
  int undo_count = 0;
  int remain_undo_cnt = 5;
  // <<< Undo 기능을 위한 변수 선언 <<<

  // >>> record, end, play 를 위한 변수 >>>
  bool is_recording = false;
  int record_frame = 0;
  int recorded_map_width, recorded_map_height;
  char recorded_map[100][SIZE][SIZE];
  int recorded_moves[100];
  char recorded_commands[100];  // (h/j/k/l/u, 프레임0은 ' ')
  char last_record_op = ' ';
  // <<< record, end, play 를 위한 변수 <<<

  // >>> 기타 명령어나 참고 메시지에 필요한 변수들 >>>
  int showing_display = GAME;
  bool is_first_game = true;
  bool is_gone_next_level = false;
  bool is_again = false;
  bool is_new = false;
  bool is_undo = false;
  bool is_stop_recording = false;
  bool is_start_recording = false;
  bool is_end_recording = false;
  bool is_play = false;
  bool is_exit = false;
  bool is_save = false;
  bool is_file_load = false;
  bool is_saved = false;
  bool is_loaded = false;
  // <<< 기타 명령어나 참고 메시지에 필요한 변수들 <<<

  // >>> f 옵션이면 저장된 게임 불러오기, 그 외에는 일반 셋업으로 >>>
  if (!load_on_start) goto SET_PLAYING_MAP_BY_PLAYING_LEVEL;

LOAD_SAVED_GAME: {
  FILE* soko_in = fopen("soko.txt", "r");
  if (soko_in == NULL) {
    if (load_on_start) {  // 초기 화면에서 불러오기인데 파일이 없는 경우
      printf("soko.txt 파일이 없습니다.\n");
      return 0;
    }
    goto START_GAME_LOOP;  // 게임 중 불러오기인데 파일이 없으면 현재 상태 유지
  }
  // 첫 줄: 이름, 레벨(0-기준), 이동 횟수
  fscanf(soko_in, "%3s %d %d", name, &playing_level, &moves_cnt);

  // 저장한 레벨의 원본 맵에서 맵 크기와 보관장소 위치를 복원
  for (int i = 0; i < SIZE; i++)
    if (maps[playing_level][0][i] == '\0') {
      fitted_map_width = i;
      break;
    }
  for (int i = 0; i < SIZE; i++)
    if (maps[playing_level][i][0] == '\0') {
      fitted_map_height = i;
      break;
    }

  // 저장된 맵을 한 줄씩 읽어 복원 ('.' -> ' ')
  char soko_line[SIZE];
  for (int i = 0; i < fitted_map_height; i++) {
    fscanf(soko_in, "%s", soko_line);
    for (int j = 0; j < fitted_map_width; j++)
      playing_map[i][j] = (soko_line[j] == '.') ? ' ' : soko_line[j];
  }
  fclose(soko_in);

  // 보관장소(box_dest_map), 남은 박스 수, 플레이어 위치 재계산
  // 보관장소는 원본 맵의 'O' 위치이며 게임 중 변하지 않음
  left_box_cnt = 0;
  for (int i = 0; i < fitted_map_height; i++)
    for (int j = 0; j < fitted_map_width; j++) {
      box_dest_map[i][j] = (maps[playing_level][i][j] == 'O');
      if (playing_map[i][j] == '@') {
        player_y = i;
        player_x = j;
      }
      // 보관장소인데 박스가 없으면 아직 채워야 할 칸
      if (box_dest_map[i][j] && playing_map[i][j] != '$') left_box_cnt++;
    }

  // 불러온 직후 undo/녹화 상태 초기화
  undo_count = 0;
  remain_undo_cnt = 5;
  is_complete_level = false;
  is_recording = false;
  record_frame = 0;
  last_record_op = ' ';
  op = ' ';

  if (load_on_start)
    load_on_start = false;  // 초기 화면 로드는 Welcome 메시지를 사용
  else
    is_loaded = true;  // 게임 진행 중 로드는 Loaded 메시지를 출력
}
  goto START_GAME_LOOP;
  // <<< f 옵션이면 저장된 게임 불러오기 <<<

SET_PLAYING_MAP_BY_PLAYING_LEVEL:
  // >>> maps -> playing_map copy & player 위치 준비 >>>
  // only depend on (playing_level)
  for (int i = 0; i < SIZE; i++)
    if (maps[playing_level][0][i] == '\0') {
      fitted_map_width = i;
      break;
    }
  for (int i = 0; i < SIZE; i++)
    if (maps[playing_level][i][0] == '\0') {
      fitted_map_height = i;
      break;
    }

  left_box_cnt = 0;
  undo_count = 0;
  remain_undo_cnt = 5;  // 레벨 시작/재시작 시 undo 기회 5번으로 초기화
  op = ' ';
  is_complete_level = false;

  // >>> 레벨 시작/재시작 시 녹화 상태 초기화 >>>
  is_recording = false;
  record_frame = 0;
  last_record_op = ' ';
  // <<< 레벨 시작/재시작 시 녹화 상태 초기화 <<<

  copy_map(playing_map, maps[playing_level], fitted_map_height,
           fitted_map_width);
  for (int i = 0; i < fitted_map_height; i++) {
    for (int j = 0; j < fitted_map_width; j++) {
      if (playing_map[i][j] == '@') {
        player_y = i, player_x = j;
      }
      if (playing_map[i][j] == 'O') {
        box_dest_map[i][j] = true;
        left_box_cnt++;
      } else {
        box_dest_map[i][j] = false;
      }
    }
  }
  // <<< maps -> playing_map copy & player 위치 준비 <<<
START_GAME_LOOP:
  while (op != EOF) {
    system("clear");
    switch (showing_display) {
      case GAME:
        show_playing_map(name, playing_level + 1, fitted_map_height,
                         fitted_map_width, playing_map);
        break;
      case HELP:
        show_help();
        break;
      case RANK:
        show_ranking(last_level);
        break;
    }

    printf("\n\n");

    // >>> 참고 메시지 >>>
    if (is_first_game) {
      is_first_game = false;
      printf("Welcome %s\n", name);
    } else if (is_gone_next_level) {
      is_gone_next_level = false;
      printf("You are in the level %d, now.\n", playing_level + 1);
    } else if (is_again) {
      is_again = false;
      printf("Again\n");
    } else if (is_new) {
      is_new = false;
      printf("Replay from level %d\n", playing_level + 1);
    } else if (is_undo) {
      is_undo = false;
      printf("Undid\n");
    } else if (is_saved) {
      is_saved = false;
      printf("Saved\n");
    } else if (is_loaded) {
      is_loaded = false;
      printf("Loaded\n");
    } else if (is_recording && last_record_op != ' ') {
      printf("recording...");
      print_command_name(last_record_op);
      printf("\n");
    } else if (is_stop_recording) {
      is_stop_recording = false;
      printf("stop recording\n");
    } else {  // 참고메시지 때문에 UI가 위아래로 움직이는 문제 해결하기 위해
      printf("\n");
    }
    // <<< 참고 메시지 <<<

    show_moves_n_command(moves_cnt, op);

    // >>> Level Clear 한 경우 >>>
    if (is_complete_level) {
      if (playing_level + 1 >= last_level) {  // 모든 Level clear 한 경우
        save_ranking(name, playing_level + 1, moves_cnt);
        printf("No more level\n");
        printf("Good bye\n");
        return 0;
      }

      char user_input;
      show_complete_screen();
      scanf("%c", &user_input);
      user_input = tolower(user_input);
      flush_stdin_line();

      bool go_next_level = (user_input == 'y') ? true : false;
      if (go_next_level) {  // >>> next level로 이동 >>>
        save_ranking(name, playing_level + 1, moves_cnt);
        playing_level++;
        moves_cnt = 0;
        is_gone_next_level = true;
        goto SET_PLAYING_MAP_BY_PLAYING_LEVEL;
      }  // <<< next level로 이동 <<<
      else {
        save_ranking(name, playing_level + 1, moves_cnt);
        printf("Good bye!!\n");
        return 0;
      }
    }
    // <<< Level Clear 한 경우 <<<

    // >>> user 입력 >>>
    op = tolower(getch());
    int dy = 0, dx = 0;
    switch (op) {
      case 'h':
        dx = -1;
        break;
      case 'j':
        dy = 1;
        break;
      case 'k':
        dy = -1;
        break;
      case 'l':
        dx = 1;
        break;
      case 'u':
        is_undo = true;
        break;
      case 'd':
        showing_display = HELP;
        break;
      case '\n':
        showing_display = GAME;
        break;
      case 't':
        showing_display = RANK;
        break;
      case 'x':
        is_exit = true;
        break;
      case 'a':
        is_again = true;
        break;
      case 'n':
        is_new = true;
        break;
      case 'r':
        is_start_recording = true;
        break;
      case 'e':
        is_end_recording = true;
        break;
      case 'p':
        is_play = true;
        break;
      case 's':
        is_save = true;
        break;
      case 'f':
        is_file_load = true;
        break;
    }
    // <<< user 입력 <<<

    // >>> 명령 처리 >>>
    if (is_exit) {
      printf("Good bye\n");
      return 0;
    } else if (dx != 0 || dy != 0) {  // >>> 이동 조작키를 눌렀을 때 >>>
      int ny = player_y + dy;
      int nx = player_x + dx;
      if (playing_map[ny][nx] == '#') continue;

      bool is_pushing_box = false;
      int box_ny, box_nx;

      if (playing_map[ny][nx] == '$') {
        box_ny = ny + dy;
        box_nx = nx + dx;
        char next_block = playing_map[box_ny][box_nx];
        if (next_block == '#' || next_block == '$')
          continue;  // early return 박스 이동 불가한 경우 바로 다시 키 입력받기

        is_pushing_box = true;
      }

      // >>> 이동 전 상태 Undo 배열에 저장 >>>
      if (undo_count == 5) {
        for (int k = 0; k < 4; k++) {
          copy_map(history_map[k], history_map[k + 1], fitted_map_height,
                   fitted_map_width);
          history_player_y[k] = history_player_y[k + 1];
          history_player_x[k] = history_player_x[k + 1];
          history_left_box_cnt[k] = history_left_box_cnt[k + 1];
        }
        undo_count = 4;
      }

      copy_map(history_map[undo_count], playing_map, fitted_map_height,
               fitted_map_width);
      history_player_y[undo_count] = player_y;
      history_player_x[undo_count] = player_x;
      history_left_box_cnt[undo_count] = left_box_cnt;
      undo_count++;
      // <<< 이동 전 상태 Undo 배열에 저장 <<<

      // >>> 박스 및 플레이어 이동 처리 >>>
      if (is_pushing_box) {
        if (box_dest_map[box_ny][box_nx] != box_dest_map[ny][nx]) {
          if (box_dest_map[box_ny][box_nx])
            left_box_cnt--;
          else
            left_box_cnt++;
        }
        playing_map[box_ny][box_nx] = '$';
      }
      playing_map[ny][nx] = '@';
      playing_map[player_y][player_x] =
          (box_dest_map[player_y][player_x]) ? 'O' : ' ';  // 이동 전 user 위치
      player_y = ny;
      player_x = nx;
      moves_cnt++;
      // <<< 박스 및 플레이어 이동 처리 <<<

      // >>> 이동 후 상태를 녹화에 추가 >>>
      if (is_recording) {
        copy_map(recorded_map[record_frame], playing_map, recorded_map_height,
                 recorded_map_width);
        recorded_moves[record_frame] = moves_cnt;
        recorded_commands[record_frame] = op;
        last_record_op = op;
        record_frame++;
        if (record_frame >= 100) {
          is_recording = false;
          is_stop_recording = true;
        }
      }
      // <<< 이동 후 상태를 녹화에 추가 <<<

      if (left_box_cnt == 0)
        is_complete_level = true;  // Level Clear는 while 문 다시 시작할 때 처리
    }  // <<< 이동 조작키를 눌렀을 때 <<<
    else if (is_again) {
      goto SET_PLAYING_MAP_BY_PLAYING_LEVEL;
    } else if (is_new) {
      playing_level = 0;
      moves_cnt = 0;
      goto SET_PLAYING_MAP_BY_PLAYING_LEVEL;
    } else if (is_undo) {
      if (remain_undo_cnt > 0 && undo_count > 0) {
        undo_count--;

        copy_map(playing_map, history_map[undo_count], fitted_map_height,
                 fitted_map_width);
        player_y = history_player_y[undo_count];
        player_x = history_player_x[undo_count];
        left_box_cnt = history_left_box_cnt[undo_count];

        moves_cnt++;
        remain_undo_cnt--;

        // >>> undo 도 녹화에 포함 >>>
        if (is_recording) {
          copy_map(recorded_map[record_frame], playing_map, recorded_map_height,
                   recorded_map_width);
          recorded_moves[record_frame] = moves_cnt;
          recorded_commands[record_frame] = 'u';
          last_record_op = 'u';
          record_frame++;
          if (record_frame >= 100) {
            is_recording = false;
            is_stop_recording = true;
          }
        }
        // <<< undo 도 녹화에 포함 <<<
      } else {
        is_undo = false;  // 실행 불가이므로 다음 턴에서 Undid 메시지 미출력
      }
    } else if (is_save) {  // 저장 처리
      is_save = false;
      save_game(name, playing_level, moves_cnt, fitted_map_height,
                fitted_map_width, playing_map);
      is_saved = true;
    } else if (is_file_load) {  // 불러오기 처리
      is_file_load = false;
      goto LOAD_SAVED_GAME;
    } else if (is_start_recording) {  // >>> 녹화 시작 처리 >>>
      is_start_recording = false;
      recorded_map_width = fitted_map_width;
      recorded_map_height = fitted_map_height;
      is_recording = true;
      record_frame = 0;
      // 현재 상태를 프레임 0으로 저장
      copy_map(recorded_map[record_frame], playing_map, recorded_map_height,
               recorded_map_width);
      recorded_moves[record_frame] = moves_cnt;
      recorded_commands[record_frame] = ' ';
      record_frame++;
      last_record_op = ' ';
    }  // <<< 녹화 시작 처리 <<<
    else if (is_end_recording) {  // >>> 녹화 종료 처리 >>>
      is_end_recording = false;
      if (is_recording) {
        is_recording = false;
        is_stop_recording = true;
      }
    }  // <<< 녹화 종료 처리 <<<
    else if (is_play) {  // >>> 녹화 재생 처리 >>>
      is_play = false;
      if (record_frame > 0) {
        for (int f = 0; f < record_frame; f++) {
          system("clear");
          show_playing_map(name, playing_level + 1, recorded_map_height,
                           recorded_map_width, recorded_map[f]);
          printf("\n\n");

          if (f == record_frame - 1) {  // 마지막 프레임
            printf("end playing...\n");
            show_moves_n_command(recorded_moves[f], ' ');
          } else if (f == 0) {  // 첫 프레임 (녹화 시작 시점 상태)
            printf("playing...\n");
            show_moves_n_command(recorded_moves[f], ' ');
          } else {
            printf("playing...");
            print_command_name(recorded_commands[f]);
            printf("\n");
            show_moves_n_command(recorded_moves[f], recorded_commands[f]);
          }

          sleep(1);
        }
        // 재생 종료 후 명령 표시를 비워서 다음 화면이 깔끔하게 보이도록
        op = ' ';
      }
    }  // <<< 녹화 재생 처리 <<<
    // <<< 명령 처리 <<<
  }

  return 0;
}

int getch(void) {
  int ch;

  struct termios buf;
  struct termios save;

  tcgetattr(0, &save);
  buf = save;

  buf.c_lflag &= ~(ICANON | ECHO);
  buf.c_cc[VMIN] = 1;
  buf.c_cc[VTIME] = 0;

  tcsetattr(0, TCSAFLUSH, &buf);

  ch = getchar();
  tcsetattr(0, TCSAFLUSH, &save);

  return ch;
}

void show_initial_screen(int last_level) {
  printf("=======================================\n");
  printf("       S   O   K   O   B   A   N       \n");
  printf("=======================================\n");
  printf("\n");
  printf("n : New Game\n");
  printf("f : File load\n");
  printf("1~%d : Level Number\n", last_level);
  printf("\n\n");
}

void show_option_screen() { printf("Input option : "); }
void show_name_screen() { printf("Input your name : "); }
void show_complete_screen() { printf("Good job! Continue (N/Y) "); }
void flush_stdin_line() {
  int ch;
  while ((ch = getchar()) != '\n' && ch != EOF);
}

bool validate_map(char map[SIZE][SIZE]) {
  int box_cnt = 0, storage_cnt = 0;
  for (int i = 0; i < SIZE; i++)
    for (int j = 0; j < SIZE; j++) {
      char block = map[i][j];
      if (block == '$')
        box_cnt++;
      else if (block == 'O')
        storage_cnt++;
      else if (block == '\0')
        continue;
      else if (!(block == '.' || block == '@' || block == '#'))
        return false;
    }
  if (storage_cnt == box_cnt) return true;
  return false;
}

void show_playing_map(char name[], int level, int height, int width,
                      char map[SIZE][SIZE]) {
  printf("================\n");
  printf(" %s in Level %d \n", name, level);
  printf("================\n");

  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      printf("%c", map[i][j]);
    }
    printf("\n");
  }
}

void show_help() {
  printf("=======================================\n");
  printf("        S O K O B A N   H E L P        \n");
  printf("=======================================\n");
  printf("h(왼쪽), j(아래), k(위), l(오른쪽)\n");
  printf("u(undo)\n");
  printf("a(again)\n");
  printf("n(new)\n");
  printf("r(record)\n");
  printf("e(record end)\n");
  printf("p(play recorded game)\n");
  printf("x(exit)\n");
  printf("s(save)\n");
  printf("f(file load)\n");
  printf("d(display help)\n");
  printf("t(top)\n");
  printf("enter(redraw map)\n");
}

void copy_map(char dest[SIZE][SIZE], char src[SIZE][SIZE], int height,
              int width) {
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      dest[i][j] = src[i][j];
    }
  }
}

void print_command_name(int op) {
  switch (op) {
    case 'h':
      printf("left");
      break;
    case 'j':
      printf("down");
      break;
    case 'k':
      printf("up");
      break;
    case 'l':
      printf("right");
      break;
    case 'u':
      printf("undo");
      break;
  }
}

void show_moves_n_command(int moves_cnt, int op) {
  printf("(Moves) %04d\n", moves_cnt);
  printf("(Command) %c\n", op);
}

void save_ranking(char name[], int level, int move_cnt) {
  char rank_name[MAX_LEVEL][5][5];
  int rank_move[MAX_LEVEL][5];
  int rank_cnt[MAX_LEVEL] = {0};

  // >>> 파일 읽기 >>>
  FILE* in = fopen("ranking.txt", "r");
  if (in != NULL) {
    int lv, mv;
    char nm[5];
    while (fscanf(in, "%d %4s %d", &lv, nm, &mv) == 3) {
      lv--;
      if (rank_cnt[lv] < 5) {
        for (int i = 0; i < 5; i++) rank_name[lv][rank_cnt[lv]][i] = nm[i];
        rank_move[lv][rank_cnt[lv]] = mv;
        rank_cnt[lv]++;
      }
    }
    fclose(in);
  }
  // <<< 파일 읽기 <

  // >>> 새 기록 추가 >>>
  int lv = level - 1;
  if (rank_cnt[lv] < 5) {
    for (int i = 0; i < 5; i++) rank_name[lv][rank_cnt[lv]][i] = name[i];
    rank_move[lv][rank_cnt[lv]] = move_cnt;
    rank_cnt[lv]++;
  }
  // <<< 새 기록 추가 <

  // >>> 이동횟수 버블정렬 >>>
  for (int i = 0; i < rank_cnt[lv] - 1; i++) {
    for (int j = 0; j < rank_cnt[lv] - 1 - i; j++) {
      if (rank_move[lv][j] > rank_move[lv][j + 1]) {
        int tmp = rank_move[lv][j];
        rank_move[lv][j] = rank_move[lv][j + 1];
        rank_move[lv][j + 1] = tmp;
        char tmp_name[5];
        for (int k = 0; k < 5; k++) tmp_name[k] = rank_name[lv][j][k];
        for (int k = 0; k < 5; k++)
          rank_name[lv][j][k] = rank_name[lv][j + 1][k];
        for (int k = 0; k < 5; k++) rank_name[lv][j + 1][k] = tmp_name[k];
      }
    }
  }
  // <<< 버블정렬 <

  // >>> 파일 쓰기 >>>
  FILE* out = fopen("ranking.txt", "w");
  for (int i = 0; i < MAX_LEVEL; i++) {
    for (int j = 0; j < rank_cnt[i]; j++) {
      fprintf(out, "%d %s %d\n", i + 1, rank_name[i][j], rank_move[i][j]);
    }
  }
  fclose(out);
  // <<< 파일 쓰기 <
}

void save_game(char name[], int playing_level, int moves_cnt, int height,
               int width, char playing_map[SIZE][SIZE]) {
  FILE* out = fopen("soko.txt", "w");
  if (out == NULL) return;
  /* 첫 줄
  이름, 레벨(0-기준), 이동 횟수
  */
  fprintf(out, "%s %d %d\n", name, playing_level, moves_cnt);
  /* 이후
  현재 맵 (공백 ' ' 은 '.' 로 저장하여 읽을 때 공백 손실 방지)
  */
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      char block = playing_map[i][j];
      fprintf(out, "%c", (block == ' ') ? '.' : block);
    }
    fprintf(out, "\n");
  }
  fclose(out);
}

void show_ranking(int last_level) {
  printf("===============\n");
  printf(" R A N K I N G \n");
  printf("===============\n");

  char rank_name[MAX_LEVEL][5][5];
  int rank_move[MAX_LEVEL][5];
  int rank_cnt[MAX_LEVEL] = {0};

  FILE* in = fopen("ranking.txt", "r");
  if (in != NULL) {
    int lv, mv;
    char nm[5];
    while (fscanf(in, "%d %4s %d", &lv, nm, &mv) == 3) {
      lv--;
      if (rank_cnt[lv] < 5) {
        for (int i = 0; i < 5; i++) rank_name[lv][rank_cnt[lv]][i] = nm[i];
        rank_move[lv][rank_cnt[lv]] = mv;
        rank_cnt[lv]++;
      }
    }
    fclose(in);
  }

  for (int i = 0; i < last_level; i++) {
    printf("*** LEVEL %d ***\n", i + 1);
    if (rank_cnt[i] == 0) {
      printf("NONE\n");
    } else {
      for (int j = 0; j < rank_cnt[i]; j++) {
        printf("%s %d\n", rank_name[i][j], rank_move[i][j]);
      }
    }
  }
}

int get_last_level_and_load_map(char maps[MAX_LEVEL][SIZE][SIZE]) {
  FILE* f;
  f = fopen("map.txt", "r");
  if (f == NULL) return -1;

  int last_level = 0;
  int height = 0;
  char line[SIZE];
  int col;
  int ch;

  while ((ch = getc(f)) != EOF) {
    col = 0;
    // 한 줄 읽기
    while (ch != '\n' && ch != EOF) {
      line[col++] = ch;
      ch = getc(f);
    }
    line[col] = '\0';

    if (strcmp(line, "s") == 0) {
      last_level++;
      height = 0;
    } else if (strcmp(line, "e") == 0) {
      break;
    } else {
      for (int i = 0; i < col; i++) {
        char c = line[i];
        if (!(c == '.' || c == '@' || c == '#' || c == '$' || c == 'O' || c == 'o')) {
          fclose(f);
          return -2;  // 공백 포함 유효하지 않은 문자 발견
        }
      }
      strcpy(maps[last_level - 1][height], line);
      height++;
    }
  }
  fclose(f);
  return last_level;
}

void input_name(char name[]) {
  while(1){
    printf("Input your name : ");
    scanf("%s", name);
    flush_stdin_line();
    int len = 0;
    int i = 1;
    while(name[len] != '\0'){
        if((name[len] < 'a' || name[len] > 'z') && (name[len] < 'A' || name[len] > 'Z')){
            i = 0;
            break;
        }
        len++;
    }
    if(i == 1 && len >= 1 && len <= 3) break;
    printf("이름을 영문자로 1~3자로 입력해주세요.\n");
  }
}