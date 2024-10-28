#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <fcntl.h>
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
void freeCommand(int wasError);
void childSignalHandler(int signum);

//Just a typedef for clarity on purpose of strings
typedef char* argument;


//Used to store one commands input
typedef struct{
    argument* args;
    int argumentCount;
} input;


//Used to store previous inputs for history
typedef struct {
  int commandCount;
  int maxCommands;
  input *entries;
} previousInputs;


//Will be used to make background task lis4t
typedef struct{
    pid_t processID;
    int status;
}background;


//Used to store formatted inputs to handle CLI operators
typedef struct{
    argument* arguments;
    int argumentCount;
    int hasOutput;
    char* outputFile;
    int append;
    int backgroundTask;
    int hasInput;
    char* inputFile;
    int pipeTo;
    int runNext;
} command;
