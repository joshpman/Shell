
#include "shell.h"
#include <termios.h>
#define bufferSize 8192
#define quit "quit"
char *readHere;
struct termios backup;
int memAll, termSet = 0;

void cleanup(int signum) {
  if (memAll)
    free(readHere);
  if (termSet)
    tcsetattr(0, TCSANOW, &backup);
  write(1, "\n", 2);
  freeArgumentList();
  exit(1);
}
void setup() {
  readHere = malloc(sizeof(char) * bufferSize);
  memAll = 1;
  char *userDir = getHomeDirectory();
  returnHome(userDir);
  setupTerminal();
  setupHelper();
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
  int nfds = 1;
  fd_set fileSet;
  char *currentDir = "] ";
  char bufferedText[512];
  memset(bufferedText, 0, sizeof(bufferedText));
  int inputLength = 0;
  setup();
  writeHeader();
  while (1) {
    memset(readHere, 0, bufferSize);
    FD_ZERO(&fileSet);
    FD_SET(0, &fileSet);
    int selectVal = select(nfds, &fileSet, NULL, NULL, NULL);
    if (FD_ISSET(0, &fileSet)) {
      ssize_t bytesIn = read(0, readHere, bufferSize);
      if (bytesIn > 0 && readHere[0]!=9)
        write(1, &readHere[0], 1); // Echoing user input
      if(readHere[0]==9){
          autocomplete(readHere, inputLength);
       }else if (readHere[0] == 0x7F) { // If input is a backspace
        if (inputLength > 0) {   // Ensuring there is data to clear
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
