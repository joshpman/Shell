
#include "shell.h"
#include <curses.h>
#include <ncurses.h>
char *readHere;
struct termios backup;
int memAll, termSet = 0;
WINDOW *win;

void cleanup(int signum) {
  if (memAll)
    free(readHere);
  if (termSet)
    tcsetattr(0, TCSANOW, &backup);
  write(1, "\n", 2);
  freeArgumentList();
  endwin();
  exit(1);
}
void setup() {
  readHere = malloc(sizeof(char) * bufferSize);
  memAll = 1;
  char *userDir = getHomeDirectory();
  returnHome(userDir);
  setupTerminal();
  // setupCurses();
  setupHelper();
}
void setupCurses() {

  win = initscr();
  // win =newwin(0,0,0,0);
  keypad(win, 1);
  noecho();
}
void setupTerminal() {

  struct termios terminalSettings;
  tcgetattr(0, &terminalSettings);
  tcgetattr(0, &backup);
  terminalSettings.c_lflag &= ~(ICANON | ECHO);
  terminalSettings.c_cc[VMIN] = 1;
  terminalSettings.c_cc[VTIME] = 0;
  tcsetattr(0, TCSANOW, &terminalSettings);
  termSet = 1;
}
int main(int argc, char **argv) {

  signal(SIGINT, cleanup);
  signal(SIGSEGV, cleanup);
  int nfds = 1;
  fd_set fileSet;
  char bufferedText[512];
  memset(bufferedText, 0, sizeof(bufferedText));
  int inputLength = 0;
  setup();
  writeHeader();
  int key = 0;
  while (0) {
    key = getch();
    switch (key) {
    case (-1):
      continue;
    case (10):
      // mvprintw(0,0,"Clicked enter\n");
      mvprintw(0, 0, "Processing command %s\n", bufferedText);
      // processCommand(bufferedText, inputLength);
      writeHeader();
      inputLength = 0;
      memset(bufferedText, 0, sizeof(bufferedText));
      break;
    case (KEY_UP):
      mvprintw(0, 0, "Up arrow pressed\n");
      break;
    case (KEY_DOWN):

    case (KEY_BACKSPACE):
      if (inputLength > 0) {
        mvwdelch(win, getcury(win), getcury(win) - 1);
        bufferedText[--inputLength] = '\0';
      }
      break;
    default:
      // mvprintw(0,0,"Key press was %d", key);
      bufferedText[inputLength++] = key;
      waddch(win, key);
    }
  }
  while (1) {
    memset(readHere, 0, bufferSize);
    FD_ZERO(&fileSet);
    FD_SET(0, &fileSet);
    select(nfds, &fileSet, NULL, NULL, NULL);
    if (FD_ISSET(0, &fileSet)) {
      ssize_t bytesIn = read(0, readHere, bufferSize);
      if (bytesIn > 0 && readHere[0] != 9)
        write(1, &readHere[0], 1); // Echoing user input
      if (readHere[0] == 9) {
        autocomplete(readHere, inputLength);
      } else if (readHere[0] == 0x7F) { // If input is a backspace
        if (inputLength > 0) {          // Ensuring there is data to clear
          write(1, "\b \b", 4);
          bufferedText[--inputLength] = '\0';
        }

      } else if (strcmp(readHere, "\n") == 0) { // If input is a newline
        strcat(bufferedText, readHere);
        inputLength += bytesIn;
        processCommand(bufferedText, inputLength);
        writeHeader();
        inputLength = 0;
        memset(bufferedText, 0, sizeof(bufferedText));
      } else { // Else
        readHere[bytesIn] = '\0';
        strcat(bufferedText, readHere);
        inputLength += bytesIn;
      }
      memset(readHere, 0, bufferSize);
    };
  }
  free(readHere);
}
