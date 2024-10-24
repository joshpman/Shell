#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
void freeArgumentList();
char *getHomeDirectory();
void changeDirectory(char *newDirectory);
void setupTerminal();
void autocomplete(char *readHere, int inputLength);
void cleanup(int signum);
void returnHome(char *homeDirectory);
void writeHeader();
void processCommand(char *buffer, int bytesRead);
void buildArgs(char *buffer, int bytesRead);
void storeArgument(int charsInArg, char *argumentBuffer, int status);
void setupHelper();
void parseCommand();
//Holds each argume
typedef char* argument;

typedef struct{
    argument* args;
    int argumentCount;
} input;

typedef struct {
  int commandCount;
  int maxCommands;
  input *entries;
} previousInputs;

typedef struct{
    
}background;

typedef struct{
    argument* arguments;
    int argumentCount;
    char* outputFile;
    int append;
    int backgroundTask;
    char* inputFile;
    int pipeTo;
    int runNext;
} command;
// argument** args;
