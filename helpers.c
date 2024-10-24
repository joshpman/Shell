#include "shell.h"
#define shellHeader "[My-Shell] "
#define historySize 128
#define maxArguments 8192
char *commands[] = {"cd"};
char **argumentListPtr = 0;
int argCount = 0;
int childPID;
previousInputs *p;
int pInit = 0;
int entriesInitalized = -1;
argument **args;
int arggCount = -1;
void setupHelper() {
  p = malloc(sizeof(previousInputs));
  p->maxCommands = historySize;
  p->commandCount = 0;
  pInit = 1;
  p->entries = malloc(historySize * sizeof(input *));
  pInit = 2;
}
void childSignalHandler(int signum) {
  kill(childPID, SIGINT);
  if (argumentListPtr != 0) {
  }
}
void freeArgumentList() {
  if (entriesInitalized >= 0) {
    for (int i = 0; i < entriesInitalized; i++) {
      int stuffToFree = p->entries[i].argumentCount;
      for (int j = 0; j < stuffToFree; j++) {
        free(p->entries[i].args[j]);
      }
      free(p->entries[i].args);
    }
    free(p->entries);
    free(p);

  } else if (pInit == 2) {
    free(p->entries);
    free(p);
  } else if (pInit == 1) {
    free(p);
  }
  storeArgument(0, 0, 3);
  if (arggCount != -1) {
    for (int i = 0; i < arggCount; i++) {
      free(*args[i]);
    }
    free(args);
  }
}
char *getHomeDirectory() {
  uid_t callingUserID = getuid();
  struct passwd *userPasswdFile = getpwuid(callingUserID);
  return userPasswdFile->pw_dir;
}
void autocomplete(char *readHere, int inputLength) {}

void executeCommand() {
  char **argumentList = p->entries[p->commandCount-1].args;
  int wordCount = p->entries[p->commandCount-1].argumentCount;
  if (strcmp(argumentList[0], "cd") == 0) {
    changeDirectory(argumentList[1]);
  } else if (strcmp(argumentList[0], "quit") == 0) {
    cleanup(0);
  } else {
    int pipeFD[2];
    pipe(pipeFD);
    pid_t child = fork();
    childPID = child;
    if (child == 0) {
      if (strcmp(argumentList[0], "man") == 0 ||
          strcmp(argumentList[0], "less")) {
        if (execvp(argumentList[0], argumentList) < 0) {
          write(2, "Command not found!\n", 20);
          exit(1);
        }
      } else {
        dup2(pipeFD[1], 1);
        close(pipeFD[1]);
        if (execvp(argumentList[0], argumentList) < 0) {
          write(2, "Command not found!\n", 20);
          exit(1);
        }
      }
    }
    if (strcmp(argumentList[0], "man") == 0 ||
        strcmp(argumentList[0], "less")) {
      signal(SIGINT, childSignalHandler);
      close(pipeFD[1]);
      char *returnData = malloc(sizeof(char) * 64000);
      waitpid(child, NULL, 0);
      int bytesRead = 0;
      while (read(pipeFD[0], &returnData[bytesRead], sizeof(returnData)) > 0) {
        write(1, returnData, strlen(returnData));
        memset(returnData, 0, sizeof(&returnData));
      }
      free(returnData);
      close(pipeFD[0]);
    } else {
      waitpid(child, NULL, 0);
    }
    // freeArgumentList();
    argumentListPtr = 0;
    argCount = 0;
    signal(SIGINT, cleanup);
  }
}

void writeHeader() { write(1, shellHeader, strlen(shellHeader)); }

// void executeCommand(){

// }
void processCommand(char *buffer, int bytesRead) {
  buildArgs(buffer, bytesRead);
  executeCommand();
}

void pushEntry(input *i) {
  if (p->commandCount == p->maxCommands) {
    memmove(&p->entries[1], &p->entries[0],
            (historySize - 1) * sizeof(input *));
  }
  p->entries[p->commandCount] = *i;
  p->commandCount != p->maxCommands ? p->commandCount++ : 0;
  entriesInitalized == -1 ? entriesInitalized = 1 : entriesInitalized++;
}

void storeArgument(int charsInArg, char *argumentBuffer, int status) {
  static input *currentCommand = NULL;
  static int commandInit = 0;
  switch (status) {
  case (0):
    currentCommand = malloc(sizeof(input));
    commandInit = 1;
    currentCommand->args = malloc(maxArguments * sizeof(argument *));
    currentCommand->argumentCount = 0;
    arggCount = 0;
    break;
  case (1):
    currentCommand->args[currentCommand->argumentCount] =
        malloc((charsInArg + 1) * sizeof(char));
    argumentBuffer[charsInArg] = '\0';
    strcpy(currentCommand->args[currentCommand->argumentCount], argumentBuffer);
    currentCommand->argumentCount++;
    break;
  case (2):
    currentCommand->args[currentCommand->argumentCount]=NULL;
    pushEntry(currentCommand);
          free(currentCommand);
          commandInit = 0;
    break;
  case (3):
    if (commandInit == 1){
      free(currentCommand);
    }
    break;
  default:
    break;
  }
}

void buildArgs(char *buffer, int bytesRead) {
  if (bytesRead == 0)
    return;
  int firstFound = 0;
  int bytesProcessed = 0;
  int charsInArg = 0;
  char argumentBuffer[256];
  storeArgument(0, 0, 0);
  while (bytesProcessed < bytesRead) {
    int currentChar = buffer[bytesProcessed++];
    switch (currentChar) {
    case (32):
      if (charsInArg != 0)
        storeArgument(charsInArg, argumentBuffer, 1);
      charsInArg = 0;
      break;
    case (10):
      storeArgument(charsInArg, argumentBuffer, 1);
      charsInArg = 0;
      break;
    default:
      argumentBuffer[charsInArg++] = (char)currentChar;

      break;
    }
  }
  storeArgument(0, 0, 2);
}
void changeDirectory(char *newDirectory) {
  if (chdir(newDirectory) < 0) {
    write(2, "cd failed: bad path!\n", 22);
  }
}

void returnHome(char *homeDirectory) {
  if (chdir(homeDirectory) < 0) {
    write(2, "Setup failed!\n", 15);
    exit(1);
  }
}
