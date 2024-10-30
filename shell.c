
#include "shell.h"
#include <curses.h>
#include <ncurses.h>
#define useCurses 0
char *readHere;
struct termios backup;
int memAll, termSet = 0;
int newStdout = 4;
WINDOW *win;
int pipeFD[2];
void checkStdout(){
  char readBuf[4096];
  ssize_t bytesIn = 0;
  while((bytesIn=read(1, readBuf, 4096))>0){
    readBuf[bytesIn] = '\0';
    waddstr(win, readBuf);
    wrefresh(win);
  }
}
void setupStdoutRedirect(){
  pipe(pipeFD);
  fcntl(pipeFD[0], F_SETFL, O_NONBLOCK);
  dup2(pipeFD[1], newStdout);
}
void cleanup(int signum) {
  if (memAll)
    free(readHere);
  if (termSet)
    tcsetattr(0, TCSANOW, &backup);
  write(1, "\n", 2);
  freeArgumentList();
  #if useCurses
  endwin();
  close(pipeFD[0]);
  close(pipeFD[1]);
  #endif
  exit(1);
}
void setup() {
  readHere = malloc(sizeof(char) * bufferSize);
  memAll = 1;
  char *userDir = getHomeDirectory();
  returnHome(userDir);
  setupTerminal();
  #if useCurses == 1
  setupCurses();
  #else
  setupHelper();
  #endif
}
void setupCurses() {
  win = initscr();
  keypad(win, 1);
  noecho();
  setupStdoutRedirect();
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
  #if useCurses
  while (1) {
    key = getch();
              checkStdout();
    switch (key) {
    case (-1):
      continue;
    case (10):
      // mvprintw(0,0,"Clicked enter\n");
      // mvprintw(0, 0, "Processing command %s\n", bufferedText);
      processCommand(bufferedText, inputLength);
      writeHeader();
      inputLength = 0;
      memset(bufferedText, 0, sizeof(bufferedText));

      break;
    case (KEY_UP):
      // mvprintw(0, 0, "Up arrow pressed\n");
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
      wrefresh(win);
    }

  }
  #else
  while (1) {
    memset(readHere, 0, bufferSize);
    FD_ZERO(&fileSet);
    FD_SET(0, &fileSet);
    select(nfds, &fileSet, NULL, NULL, NULL);
    if (FD_ISSET(0, &fileSet)) {
      ssize_t bytesIn = read(0, readHere, bufferSize);
      // printf("Buffer size is %lu\n", bytesIn);
      switch(bytesIn){
        case(0):
        continue;
        case(3):
          if(readHere[0] ==27 && readHere[1]==91){
            if(readHere[2]==65){
              void* histVal = fetchHistory(1);
              if(histVal==0) goto parse;
              input history = *(input*)histVal;
              for(int i = 0; i<history.argumentCount; i++){
                printf("%s ", history.args[i]);
              }
              // printf("\n");
              //up arrow
            }else if(readHere[2]==66){
                void* histVal = fetchHistory(0);
              if(histVal==0) goto parse;
              input history = *(input*)histVal;
              // printf("Down arrow pressed!\n");
              //down arrow
            }else{
              goto parse;
            }
          }
          continue;
          // printf("Key presses were %d %d %d\n", readHere[0],readHere[1],readHere[2]);
        case(1):
          goto parse;
        case(-1):
          write(2, "Error occured! Exiting\n", 24);
          cleanup(1);
          exit(1);
      }
      parse:
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
  #endif
}
