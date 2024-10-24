#include "shell.h"
#define shellHeader "[My-Shell] "
#define historySize 128
#define maxArguments 128
#define maxCommandChain 8
#define maxSubArgs 8
char *commands[] = {"cd"};
char **argumentListPtr = 0;
int argCount = 0;
int childPID;
previousInputs *p;
int pInit = 0;
int entriesInitalized = -1;
command **commandHolder;
int commandHolderInit = -1;
int commandHolderEntriesUsed = 0;
void setupHelper() {
  p = malloc(sizeof(previousInputs));
  p->maxCommands = historySize;
  p->commandCount = 0;
  pInit = 1;
  p->entries = malloc(historySize * sizeof(input *));
  pInit = 2;
}
void executeCommand(){
  // printf("Command list has %d entries\n", commandHolderEntriesUsed);
  // printf("This function has %d for pipeTo\n", currentEntry->pipeTo);
  // printf("This function has %s for outputFile\n", currentEntry->outputFile);
  command *currentEntry = commandHolder[0];
  int argumentCount = currentEntry->argumentCount;
  int pipeTo = currentEntry->pipeTo;
  char* outputFile = currentEntry->outputFile;
  char* inputFile = currentEntry->inputFile;
  int argCount = currentEntry->argumentCount;
  argument* arguments = currentEntry->arguments;
  int append = currentEntry->append;
  int runNext = currentEntry->runNext;
  for(int i = 0; i<argumentCount; i++){
    printf("argument #%d is %s\n", i, arguments[i]);
  }
  // freeCommand(0);


}
void childSignalHandler(int signum) { kill(childPID, SIGINT); }
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
  freeCommand(-1);
}
char *getHomeDirectory() {
  uid_t callingUserID = getuid();
  struct passwd *userPasswdFile = getpwuid(callingUserID);
  return userPasswdFile->pw_dir;
}
void autocomplete(char *readHere, int inputLength) {}

void executeCommand2() {
  char **argumentList = p->entries[p->commandCount - 1].args;
  int wordCount = p->entries[p->commandCount - 1].argumentCount;
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

void processCommand(char *buffer, int bytesRead) {
  buildArgs(buffer, bytesRead);
  parseCommand();
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

/*
I fought pointers for like an hour to get this function to work
*/
void storeArgument(int charsInArg, char *argumentBuffer, int status) {
  static input *currentCommand = NULL;
  static int commandInit = 0;
  switch (status) {
  case (0):
    currentCommand = malloc(sizeof(input));
    commandInit = 1;
    currentCommand->args = malloc(maxArguments * sizeof(argument *));
    currentCommand->argumentCount = 0;
    break;
  case (1):
    currentCommand->args[currentCommand->argumentCount] =
        malloc((charsInArg + 1) * sizeof(char));
    argumentBuffer[charsInArg] = '\0';
    strcpy(currentCommand->args[currentCommand->argumentCount], argumentBuffer);
    currentCommand->argumentCount++;
    break;
  case (2):
    currentCommand->args[currentCommand->argumentCount] = NULL;
    pushEntry(currentCommand);
    free(currentCommand);
    commandInit = 0;
    break;
  case (3):
    if (commandInit == 1) {
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
void storeCommand(command *command, int pos) {
  commandHolder[pos] = command;
  commandHolderInit++;
}
void freeCommand(int wasError) {
  for (int i = 0; i < commandHolderInit; i++) {
    for(int j = 0; j<commandHolder[i]->argumentCount; j++){
      free(commandHolder[i]->arguments[j]);
    }
    free(commandHolder[i]->arguments);
    free(commandHolder[i]->outputFile);
    free(commandHolder[i]->inputFile);
  }
  for (int i = 0; i < maxCommandChain; i++) {
    free(commandHolder[i]);
  }
    if(wasError==-1){
    free(commandHolder);
  }
  if (wasError) {
    write(2, "Parse error!\n", 14);
  }
}

/*
Another hour of hand-to-hand combat with pointers and this is still garbage code
*/
void parseCommand() {
  int wordsToCheck = p->entries[p->commandCount - 1].argumentCount;
  commandHolder = malloc(sizeof(command *) * maxCommandChain);
  commandHolderInit = 0;
  for(int i = 0; i<maxCommandChain; i++){
    commandHolder[i] = malloc(sizeof(command));
        memset(commandHolder[i], 0, sizeof(*commandHolder[i]));
    commandHolder[i]->arguments =  malloc(sizeof(argument *) * maxSubArgs);
      commandHolderInit++;
  }
  char *wordBuffer[wordsToCheck];
  memset(&wordBuffer, 0, sizeof(wordBuffer));

  int bufPointer = 0;
  int currentCommand = 0;
  commandHolderEntriesUsed = 1;
  for (int i = 0; i < wordsToCheck; i++) {
    // printf("I: %d, words to check: %d\n", i, wordsToCheck);
    char *currentWord = p->entries[p->commandCount - 1].args[i];
    int wordLength = strlen(currentWord);
    if (wordLength >= 1) {
      switch (currentWord[0]) {
      case (60): // Represents <
        if (wordLength == 1 && i != wordsToCheck - 1) {
          commandHolder[currentCommand]->inputFile =
              malloc(sizeof(char) *
                     strlen(p->entries[p->commandCount - 1].args[i + 1]));
          commandHolder[currentCommand]->inputFile =
              p->entries[p->commandCount - 1].args[i + 1];
        } else {
          freeCommand(1);
          return;
        }
        break;
      case (62): // Represents >
        if (wordLength == 1 && i != wordsToCheck - 1) {
          commandHolder[currentCommand]->outputFile =
              malloc(sizeof(char) *
                     strlen(p->entries[p->commandCount - 1].args[i + 1]));
          commandHolder[currentCommand]->outputFile =
              p->entries[p->commandCount - 1].args[i + 1];

        } else if (wordLength == 2 && currentWord[1] == 62 &&
                   i != wordsToCheck - 1) {
          commandHolder[currentCommand]->outputFile =
              malloc(sizeof(char) *
                     strlen(p->entries[p->commandCount - 1].args[i + 1]));
          commandHolder[currentCommand]->outputFile =
              p->entries[p->commandCount - 1].args[i + 1];
          commandHolder[currentCommand]->append = 1;
          currentCommand ++;
                    commandHolderEntriesUsed++;

        } else {
          freeCommand(1);
          return;
        }
        break;
      case (38): // Repesents &
        if (wordLength == 1) {
          commandHolder[currentCommand]->backgroundTask = 1;
        } else if (wordLength == 2 && i != wordsToCheck - 1) {
          commandHolder[currentCommand]->runNext = currentCommand+1;
          currentCommand++;
          commandHolderEntriesUsed++;
        } else {
          freeCommand(1);
          return;
        }
        break;
      case (124): // Represents |
        if (wordLength > 1 || i == wordsToCheck - 1) {
          freeCommand(1);
          return;
        } else {
          commandHolder[currentCommand]->pipeTo = currentCommand+1;
          currentCommand++;
          commandHolderEntriesUsed++;
        }
        break;
      default:
        commandHolder[currentCommand]->arguments[bufPointer] = malloc(sizeof(char) * (wordLength + 1)); // Allocate memory for string + null terminator
            currentWord[strlen(currentWord)] = '\0';
        strcpy(commandHolder[currentCommand]->arguments[bufPointer], currentWord);
        bufPointer++;
        commandHolder[currentCommand]->argumentCount++;
        break;
      }
    }
  }
}
/*
How to add pipes & I/O Redirection

1. Need to parse the command list and look for |, >,>>,<,&&
2. Sort each command into the command struct


How to handle stuff like "./encode < file | ./decode > decodedFile"

*/