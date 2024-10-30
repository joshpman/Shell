#include <fcntl.h>
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

#define maxSubArgs 8
#define shellHeader "[My-Shell] "
#define historySize 128
#define maxArguments 128
#define maxCommandChain 8
#define bufferSize 8192
void freeArgumentList();
void setupCurses();
void setupTerminal();
void autocomplete(char *readHere, int inputLength);
void cleanup(int signum);
void writeHeader();
void processCommand(char *buffer, int bytesRead);
void buildArgs(char *buffer, int bytesRead);
void storeArgument(int charsInArg, char *argumentBuffer, int status);
void setupHelper();
void parseCommand();
void freeCommand(int wasError);
void childSignalHandler(int signum);
static inline void changeDirectory(char *newDirectory);
static inline char *getHomeDirectory();
static inline void returnHome(char *homeDirectory);

static inline char *getHomeDirectory() {
  uid_t callingUserID = getuid();
  struct passwd *userPasswdFile = getpwuid(callingUserID);
  return userPasswdFile->pw_dir;
}

static inline void returnHome(char *homeDirectory) {
  if (chdir(homeDirectory) < 0) {
    write(2, "Setup failed!\n", 15);
    exit(1);
  }
}
static inline void changeDirectory(char *newDirectory) {
if (chdir(newDirectory) < 0) {
    write(2, "cd failed: bad path!\n", 22);
  }
}

// Just a typedef for clarity on purpose of strings
typedef char *argument;

// Used to store one commands input
typedef struct {
  argument *args;
  int argumentCount;
} input;

// Used to store previous inputs for history
typedef struct {
  int commandCount;
  int maxCommands;
  input *entries;
} previousInputs;

// Will be used to make background task lis4t
typedef struct {
  pid_t processID;
  int status;
} background;

// Used to store formatted inputs to handle CLI operators
typedef struct {
  argument *arguments;
  int argumentCount;
  int hasOutput;
  char *outputFile;
  int append;
  int backgroundTask;
  int hasInput;
  char *inputFile;
  int pipeTo;
  int runNext;
} command;
