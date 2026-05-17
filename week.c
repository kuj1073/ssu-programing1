//녹화 중 클리어 되어서 맵이 바뀌었을 경우 해결해야 함. 
//또는 녹화가 전 레벨에서 시작되고 끝나는 경우에도 해결해야함 전 레벨과 현재 레벨의 '0' 위치가 겹치는 문제 생김 
//랭킹 기능도 만들어야 함 
// undo 기능

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <termio.h>
#include <unistd.h>
#define SIZE 30
#define MAX_LEVEL 3

int getch(void);
void show_inital_screen();
void show_option_screen();
void show_name_screen();
void show_complete_screen();
void flush_stdin_line();
bool validate_map(char[SIZE][SIZE]);
void show_playing_map(char[], int, int, int, char[SIZE][SIZE]);
void save_ranking(char[], int, int);
void show_ranking();
void show_player_move_cnt(int);
void show_help();

int main(void) {
  char maps[MAX_LEVEL][SIZE][SIZE] = {{
                                         /* "    #####          ",
                                          "    #   #          ",
                                          "    #$  #          ",
                                          "  ###  $##         ",
                                          "  #  $ $ #         ",
                                          "### # ## #   ######",
                                          "#   # ## #####  OO#",
                                          "# $  $          OO#",
                                          "##### ### #@##  OO#",
                                          "    #     #########",
                                          "    #######        ",*/
                                          "#######",
                                          "#@    #",
                                          "#  $  #",
                                          "#  O  #",
                                          "#  $  #",
                                          "#  O  #",
                                          "#######"
                                      },
                                      {
                                          "############  ",
                                          "#OO  #     ###",
                                          "#OO  # $  $  #",
                                          "#OO  #$####  #",
                                          "#OO    @ ##  #",
                                          "#OO  # #  $ ##",
                                          "###### ##$ $ #",
                                          "  # $  $ $ $ #",
                                          "  #    #     #",
                                          "  ############",
                                      },
                                      {
                                          "        ######## ",
                                          "        #     @# ",
                                          "        # $#$ ## ",
                                          "        # $  $#  ",
                                          "        ##$ $ #  ",
                                          "######### $ # ###",
                                          "#OOOO  ## $  $  #",
                                          "##OOO    $  $   #",
                                          "#OOOO  ##########",
                                          "########         ",
                                      }};
  // >>> validate map >>>
  for (int k = 0; k < MAX_LEVEL; k++) {
    if (!validate_map(maps[k])) {
      printf("Wrong level %d map\n", k + 1);
      return 0;
    }
  }
  // <<< validate map <

  char option;
  char name[5]; 
  int playing_level;
  int player_move_cnt = 0;
  char record[100];
  int record_cnt = 0;
  int undo_total = 0;

  show_inital_screen();
  show_option_screen();
  scanf("%c", &option);
  option = tolower(option);
  show_name_screen();
  scanf("%s", name); 
  flush_stdin_line();

  switch (option) {
    case 'n':
      playing_level = 0;
      break;
    case 'f':
      // TODO
      break;
      return 0;
    case '1':
    case '2':
    case '3':
      playing_level = option - '1';
      player_move_cnt = 0;
      break;
  }
  // >>> play에서 필요한 변수들 (SET_PLAYING_MAP_BY_PLAYING_LEVEL 에서 사용하는
  // 변수들) >>>
  char playing_map[SIZE][SIZE];
  bool box_dest_map[SIZE][SIZE];
  int fitted_map_width, fitted_map_height;
  int left_box_cnt;
  int player_y, player_x;
  bool is_complete_level;
  // <<< play에서 필요한 변수들 <

  // >>> 기타 명령어나 참고 메시지에 필요한 변수들 >>>
  bool is_first_game = true;
  bool is_gone_next_level = false;
  bool is_showing_help = false;
  bool is_again = false;
  bool is_new = false;
  bool is_recording = false;
  bool is_playing_recording = false;
  int play_idx = 0;

  // 녹화 시작 시점 백업 (p 눌렀을 때 재생 시작점으로 돌아가기 위해)
  char backup_map[SIZE][SIZE];
  int backup_player_y, backup_player_x;
  int backup_left_box_cnt;
  int backup_player_move_cnt;
  int backup_playing_level;

  // 재생 시작 시점 백업 (재생 끝나고 원래 상태로 돌아가기 위해)
  char current_map[SIZE][SIZE];
  int current_player_y, current_player_x;
  int current_left_box_cnt;
  int current_player_move_cnt;
  int current_playing_level;

  char undo_map[5][SIZE][SIZE];
  int undo_player_y[5];
  int undo_player_x[5];
  int undo_left_box_cnt[5];
  int undo_cnt = 0;
  // <<< 기타 명령어나 참고 메시지에 필요한 변수들 <

SET_PLAYING_MAP_BY_PLAYING_LEVEL:
  undo_cnt = 0;
  undo_total = 0;
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
  is_complete_level = false;
  for (int i = 0; i < fitted_map_height; i++) {
    for (int j = 0; j < fitted_map_width; j++) {
      playing_map[i][j] = maps[playing_level][i][j];
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
  // <<< maps -> playing_map copy & player 위치 준비 <

  while (1) {
    if (is_playing_recording) usleep(500000);
    system("clear");
    if (is_showing_help) {
      show_help();
    } else {
      show_playing_map(name, playing_level + 1, fitted_map_height, fitted_map_width, playing_map);
      printf("\n\n");
      show_player_move_cnt(player_move_cnt);
      if(is_recording) printf("recording...\n");
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
      printf("Replay from level 1\n");
    } else if (is_playing_recording) {
      printf("playing...\n");
    } else {  // 참고메시지 때문에 UI가 위아래로 움직이는 문제 해결하기 위해
      printf("\n");
    }
    // <<< 참고 메시지 <

    // >>> Level Clear 한 경우 >>>
    if (is_complete_level) {
      if (playing_level + 1 >= MAX_LEVEL) {  // 모든 Level clear 한 경우
        save_ranking(name, playing_level + 1, player_move_cnt);
        printf("No more level\n");
        printf("Good bye\n");
        return 0;
      }

      if (!is_playing_recording) {  // 재생 중이 아닐 때만 사용자 입력 받기
        char user_input;
        show_complete_screen();
        scanf("%c", &user_input);
        user_input = tolower(user_input);
        flush_stdin_line();

        bool go_next_level = (user_input == 'y') ? true : false;
        if (go_next_level) {  // >>> next level로 이동 >>>
          save_ranking(name, playing_level + 1, player_move_cnt);
          player_move_cnt = 0;
          playing_level++;
          is_gone_next_level = true;
          goto SET_PLAYING_MAP_BY_PLAYING_LEVEL;
        }  // <<< next level로 이동 <
        else {
          save_ranking(name, playing_level + 1, player_move_cnt);
          printf("Good bye!!\n");
          return 0;
        }
      }
    }
    // <<< Level Clear 한 경우 <

    // >>> user 입력 >>>
    int dy = 0, dx = 0;

    // >>> 재생 중일 때 >>>
    if (is_playing_recording) {
      if (play_idx < record_cnt) {
        int rec = record[play_idx++];
        if (rec == 'h') dx = -1;
        else if (rec == 'j') dy = 1;
        else if (rec == 'k') dy = -1;
        else if (rec == 'l') dx = 1;
        else if (rec == '!') {
          // 레벨 전환 재현
          playing_level++;
          for (int i = 0; i < SIZE; i++)
            if (maps[playing_level][0][i] == '\0') { fitted_map_width = i; break; }
          for (int i = 0; i < SIZE; i++)
            if (maps[playing_level][i][0] == '\0') { fitted_map_height = i; break; }
          left_box_cnt = 0;
          is_complete_level = false;
          for (int i = 0; i < fitted_map_height; i++)
            for (int j = 0; j < fitted_map_width; j++) {
              playing_map[i][j] = maps[playing_level][i][j];
              if (playing_map[i][j] == '@') { player_y = i; player_x = j; }
              if (playing_map[i][j] == 'O') { box_dest_map[i][j] = true; left_box_cnt++; }
              else box_dest_map[i][j] = false;
            }
          continue;
        }
      } else {
        // 재생 끝 -> 현재 상태로 복원
        is_playing_recording = false;
        player_y = current_player_y;
        player_x = current_player_x;
        left_box_cnt = current_left_box_cnt;
        player_move_cnt = current_player_move_cnt;
        playing_level = current_playing_level;
        for (int i = 0; i < SIZE; i++)
            for (int j = 0; j < SIZE; j++)
                playing_map[i][j] = current_map[i][j];
        for (int i = 0; i < fitted_map_height; i++)
            for (int j = 0; j < fitted_map_width; j++)
                box_dest_map[i][j] = (maps[current_playing_level][i][j] == 'O') ? true : false;
        printf("end playing...\n");
        sleep(1);
        continue;
      }
    }
    // <<< 재생 중일 때 <
    else {
      int op = tolower(getch());
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
          if (undo_cnt > 0 && undo_total < 5) {
            undo_cnt--;
            undo_total++;
            player_y = undo_player_y[undo_cnt];
            player_x = undo_player_x[undo_cnt];
            left_box_cnt = undo_left_box_cnt[undo_cnt];
            for (int r = 0; r < SIZE; r++)
              for (int c = 0; c < SIZE; c++)
                playing_map[r][c] = undo_map[undo_cnt][r][c];
            player_move_cnt++;
            is_complete_level = false;
          }
          break;
        case 'd':
          is_showing_help = true;
          break;
        case '\n':
          is_showing_help = false;
          break;
        case 'x':
          printf("Good bye\n");
          return 0;
          break;
        case 'a':
          is_again = true;
          is_recording = false;
          record_cnt = 0;
          break;
        case 'n':
          is_new = true;
          is_recording = false;
          record_cnt = 0;
          break;
        case 'r':
          is_recording = true;
          record_cnt = 0;
          // 녹화 시작 시점 맵 백업
          backup_player_y = player_y;
          backup_player_x = player_x;
          backup_left_box_cnt = left_box_cnt;
          backup_player_move_cnt = player_move_cnt;
          backup_playing_level = playing_level;
          for (int i = 0; i < SIZE; i++)
            for (int j = 0; j < SIZE; j++)
              backup_map[i][j] = playing_map[i][j];
          break;
        case 'e':
          printf("end recording\n");
          is_recording = false;
          break;
        case 'p':
            is_playing_recording = true;
            play_idx = 0;
            // 현재 상태 백업
            current_player_x = player_x;
            current_player_y = player_y;
            current_left_box_cnt = left_box_cnt;
            current_player_move_cnt = player_move_cnt;
            current_playing_level = playing_level;
            for (int i = 0; i < SIZE; i++)
                for (int j = 0; j < SIZE; j++)
                    current_map[i][j] = playing_map[i][j];
            // 녹화 시작 시점으로 맵 복원
            player_y = backup_player_y;
            player_x = backup_player_x;
            left_box_cnt = backup_left_box_cnt;
            player_move_cnt = backup_player_move_cnt;
            playing_level = backup_playing_level;
            for (int i = 0; i < SIZE; i++)
                for (int j = 0; j < SIZE; j++)
                    playing_map[i][j] = backup_map[i][j];
            for (int i = 0; i < fitted_map_height; i++)
                for (int j = 0; j < fitted_map_width; j++)
                    box_dest_map[i][j] = (maps[backup_playing_level][i][j] == 'O') ? true : false;
            break;
        case 't':
          show_ranking();
          sleep(2);
          break;
      }

      if (is_recording && (dx != 0 || dy != 0)) {
        if (record_cnt < 100) {
          record[record_cnt++] = op;
        } else {
          is_recording = false;
        }
      }
    }
    // <<< user 입력 <

    if (dx != 0 || dy != 0) {
      int ny = player_y + dy;
      int nx = player_x + dx;
      if (playing_map[ny][nx] == '#') continue;
      if (playing_map[ny][nx] == '$') {
        int box_ny = ny + dy;
        int box_nx = nx + dx;
        char next_block = playing_map[box_ny][box_nx];
        if (next_block == '#' || next_block == '$') continue;

        // >>> undo 저장 (박스 이동 전) >>>
        if (undo_cnt == 5) {
          for (int i = 0; i < 4; i++) {
            undo_player_y[i] = undo_player_y[i + 1];
            undo_player_x[i] = undo_player_x[i + 1];
            undo_left_box_cnt[i] = undo_left_box_cnt[i + 1];
            for (int r = 0; r < SIZE; r++)
              for (int c = 0; c < SIZE; c++)
                undo_map[i][r][c] = undo_map[i + 1][r][c];
          }
          undo_cnt = 4;
        }
        undo_player_y[undo_cnt] = player_y;
        undo_player_x[undo_cnt] = player_x;
        undo_left_box_cnt[undo_cnt] = left_box_cnt;
        for (int r = 0; r < SIZE; r++)
          for (int c = 0; c < SIZE; c++)
            undo_map[undo_cnt][r][c] = playing_map[r][c];
        undo_cnt++;
        // <<< undo 저장 <

        if (box_dest_map[box_ny][box_nx] != box_dest_map[ny][nx]) {
          if (box_dest_map[box_ny][box_nx]) left_box_cnt--;
          else left_box_cnt++;
        }
        playing_map[box_ny][box_nx] = '$';
      } else {
        // 박스 없을 때
        if (undo_cnt == 5) {
          for (int i = 0; i < 4; i++) {
            undo_player_y[i] = undo_player_y[i + 1];
            undo_player_x[i] = undo_player_x[i + 1];
            undo_left_box_cnt[i] = undo_left_box_cnt[i + 1];
            for (int r = 0; r < SIZE; r++)
              for (int c = 0; c < SIZE; c++)
                undo_map[i][r][c] = undo_map[i + 1][r][c];
          }
          undo_cnt = 4;
        }
        undo_player_y[undo_cnt] = player_y;
        undo_player_x[undo_cnt] = player_x;
        undo_left_box_cnt[undo_cnt] = left_box_cnt;
        for (int r = 0; r < SIZE; r++)
          for (int c = 0; c < SIZE; c++)
            undo_map[undo_cnt][r][c] = playing_map[r][c];
        undo_cnt++;
      }

      playing_map[ny][nx] = '@';
      playing_map[player_y][player_x] = (box_dest_map[player_y][player_x]) ? 'O' : ' ';
      player_y = ny;
      player_x = nx;
      if (!is_playing_recording) player_move_cnt++;
      if (left_box_cnt == 0) {
        is_complete_level = true;
        if (is_recording) {
          if (record_cnt < 100) record[record_cnt++] = '!';
          else is_recording = false;
        }
      }
    } else if (is_again) {
      goto SET_PLAYING_MAP_BY_PLAYING_LEVEL;
    } else if (is_new) {
      playing_level = 0;
      player_move_cnt = 0;
      goto SET_PLAYING_MAP_BY_PLAYING_LEVEL;
    }
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

void show_inital_screen() {
  printf("=======================================\n");
  printf("       S   O   K   O   B   A   N       \n");
  printf("=======================================\n");
  printf("\n");
  printf("n : New Game\n");
  printf("f : File load\n");
  printf("1~3 : Level Number\n");
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
    }
  if (storage_cnt == box_cnt) return true;
  return false;
}

void show_playing_map(char name[], int level, int height, int width, char map[SIZE][SIZE]) {
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

void save_ranking(char name[], int level, int move_cnt) {
    char rank_name[MAX_LEVEL][5][5]; 
    int rank_move[MAX_LEVEL][5];
    int rank_cnt[MAX_LEVEL] = {0};

    // >>> 파일 읽기 >>>
    FILE *in = fopen("ranking.txt", "r");
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
                for (int k = 0; k < 5; k++) rank_name[lv][j][k] = rank_name[lv][j + 1][k];
                for (int k = 0; k < 5; k++) rank_name[lv][j + 1][k] = tmp_name[k];
            }
        }
    }
    // <<< 버블정렬 <

    // >>> 파일 쓰기 >>>
    FILE *out = fopen("ranking.txt", "w");
    for (int i = 0; i < MAX_LEVEL; i++) {
        for (int j = 0; j < rank_cnt[i]; j++) {
            fprintf(out, "%d %s %d\n", i + 1, rank_name[i][j], rank_move[i][j]);
        }
    }
    fclose(out);
    // <<< 파일 쓰기 <
}

void show_ranking() {
    printf("===============\n");
    printf("  R A N K I N G\n");
    printf("===============\n");

    char rank_name[MAX_LEVEL][5][5]; 
    int rank_move[MAX_LEVEL][5];
    int rank_cnt[MAX_LEVEL] = {0};

    FILE *in = fopen("ranking.txt", "r");
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

    for (int i = 0; i < MAX_LEVEL; i++) {
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

void show_player_move_cnt(int player_move_cnt) { printf("(Moves) %4d\n", player_move_cnt); }

void show_help() {
  printf("=======================================\n");
  printf("        S O K O B A N   H E L P        \n");
  printf("=======================================\n");
  printf("h(왼쪽), j(아래), k(위), l(오른쪽)\n");
  printf("u(undo)\n");
  printf("a(again)\n");
  printf("n(new)\n");
  printf("x(exit)\n");
  printf("s(save)\n");
  printf("f(file load)\n");
  printf("d(display help)\n");
  printf("t(top)\n");
  printf("r(record)\n");
  printf("e(record end)\n");
  printf("p(play recorded game)\n");
  printf("enter(redraw map)\n");
}