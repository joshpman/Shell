#include "shell.h"
#include <fcntl.h>
#define shellHeader "[My-Shell] "
#define historySize 128
#define maxArguments 128
#define maxCommandChain 8
command *commandHolder;
previousInputs *p;

// Globals for managing exec's and signals
sig_atomic_t action = 0;
int childPID;

// Globals for managing the argument list constructed
char **argumentListPtr = 0;
int argCount = 0;

// Globals for managing the previous entries
int pInit = 0;
int entriesInitalized = -1;

// Globals for managing the commandHolder
int commandHolderInit = -1;
int commandHolderEntriesUsed = 0;
int commandsToExecute = 1;

void setupHelper() {
  p = malloc(sizeof(previousInputs));
  p->maxCommands = historySize;
  p->commandCount = 0;
  pInit = 1;
  p->entries = malloc(historySize * sizeof(input *));
  pInit = 2;
}

void executeCommand() {

  command currentEntry = commandHolder[0];
  argument *arguments = currentEntry.arguments;

  // CD is a fake command so we have to catch it here and use chdir
  if (strcmp(arguments[0], "cd") == 0) {
    changeDirectory(arguments[1]);
    freeCommand(0);
    return;
  }
  // Exit commands
  if (!strcmp(arguments[0], "quit") || !strcmp(arguments[0], "exit")) {
    cleanup(1);
  }

  // Signal action - if 0, run normally, else we exit
  action = 0;

  // Setting up pipes to manage sending data between different commands
  int pipeFD[2], previousPipeFD[2];
  pipeFD[0] = pipeFD[1] = previousPipeFD[0] = previousPipeFD[1] = -1;

  // Setting up FD's to manage IO Redirection
  int outputFD = 1;
  int inputFD = 0;

  // Declaring values to use to backup stdin/stdout file descriptor
  int stdinBackup = -1;
  int stdoutBackup = -1;

  // First exec flag
  int first = 1;

  // Various state flags to use in do-while logic
  int pipeFDset = 0;
  int needPipe = 0;
  int pipeWriteBackup = 0;

  // Iteration counter for loop
  int i = 0;
  do {
    needPipe = 0;
    command currentCommand = commandHolder[i];

    if (currentCommand.pipeTo != 0 && i + 1 < commandsToExecute) {
      pipe(pipeFD);
      commandHolder[i + 1].hasInput = 2;
      needPipe = 1;
    }

    childPID = fork();
    if (childPID == 0) {

      if (currentCommand.hasInput > 1) {
        dup2(atoi(currentCommand.inputFile), 0);
      } else if (currentCommand.hasInput == 1) {
        inputFD = open(currentCommand.inputFile, O_RDONLY);
        if (inputFD < 0) {
          perror("Error opening input file");
          exit(1);
        }
        dup2(inputFD, 0);
        close(inputFD);
      }

      if (currentCommand.hasOutput == 1) {
        outputFD = open(currentCommand.outputFile,
                        O_CREAT | (currentCommand.append ? O_APPEND : O_TRUNC) |
                            O_WRONLY,
                        0644);
        if (outputFD < 0) {
          perror("Error opening output file");
          exit(1);
        }
        dup2(outputFD, 1);
        close(outputFD);
      } else if (needPipe) {
        dup2(pipeFD[1], 1);
        close(pipeFD[0]);
        close(pipeFD[1]);
      }

      char **execArgs =
          malloc(sizeof(char *) * (currentCommand.argumentCount + 1));
      for (int j = 0; j < currentCommand.argumentCount; j++) {
        char writeBuf[128];
        execArgs[j] = currentCommand.arguments[j];
      }
      execArgs[currentCommand.argumentCount] = NULL;
      if ((execvp(execArgs[0], execArgs)) < 0) {
        perror("Exec failed");
        free(execArgs);
        exit(1);
      } else {
        free(execArgs);
        exit(0);
      }
    }

    if (needPipe) {
      close(pipeFD[1]);
      char fdBuf[8];
      sprintf(fdBuf, "%d", pipeFD[0]);
      commandHolder[i + 1].inputFile = strdup(fdBuf);
    }
    signal(SIGINT, childSignalHandler);
    wait(NULL);
    signal(SIGINT, cleanup);
    if (action == 1)
      break;
    i++;
  } while (i < commandsToExecute);
  dup2(stdinBackup, 0);
  dup2(stdoutBackup, 1);
  freeCommand(-1);
}

void childSignalHandler(int signum) {
  action = 1;
  kill(childPID, SIGINT);
}

// Pro memory freeing just don't read any of the code
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
  if (commandHolderInit != -1)
    freeCommand(-1);
}

char *getHomeDirectory() {
  uid_t callingUserID = getuid();
  struct passwd *userPasswdFile = getpwuid(callingUserID);
  return userPasswdFile->pw_dir;
}

/*
So many bugs to fix before I write this
*/
void autocomplete(char *readHere, int inputLength) {}

// Why is this a function
void writeHeader() { write(1, shellHeader, strlen(shellHeader)); }

// Nice logic with so much pain in each of these functions
void processCommand(char *buffer, int bytesRead) {
  buildArgs(buffer, bytesRead);
  parseCommand();
  executeCommand();
}

// Weird stack thingy using memmove
void pushEntry(input *i) {
  if (p->commandCount == p->maxCommands) {
    memmove(&p->entries[1], &p->entries[0],
            (historySize - 1) * sizeof(input *));
  }

  memcpy(&p->entries[p->commandCount], i, sizeof(*i));

  // Ternary hell
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
    argumentBuffer[charsInArg] = '\0';
    currentCommand->args[currentCommand->argumentCount] = strdup(argumentBuffer);
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

// Parse input by splitting arguments with spaces/newlines between them and
// storing them in an input struct
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
void storeCommand(command command, int pos) {
  commandHolder[pos] = command;
  commandHolderInit++;
}

/*
American Function(cuz its the land of the free)
*/
void freeCommand(int wasError) {
  if (commandHolderInit == -1) {
    return;
  }
  for (int i = 0; i < commandHolderInit; i++) {
    for (int j = 0; j < commandHolder[i].argumentCount; j++) {

      free(commandHolder[i].arguments[j]);
    }
    free(commandHolder[i].arguments);
    if (commandHolder[i].hasOutput)
      free(commandHolder[i].outputFile);
    if (commandHolder[i].hasInput)
      free(commandHolder[i].inputFile);
  }
  free(commandHolder);
  commandHolderInit = -1;
  commandHolderEntriesUsed = 0;
  if (wasError == 1) {
    write(2, "Parse error!\n", 14);
  }
}

void storeInput(int commandHolderIndex, int commandIndex) {
  commandHolder[commandHolderIndex].outputFile =
      strdup(p->entries[p->commandCount - 1].args[commandIndex]);

  commandHolder[commandHolderIndex].hasInput = 1;
}

void storeOutput(int commandHolderIndex, int commandIndex, int append) {
  commandHolder[commandHolderIndex].outputFile =
      strdup(p->entries[p->commandCount - 1].args[commandIndex]);

  commandHolder[commandHolderIndex].hasOutput = 1;
  if (append == 1)
    commandHolder[commandHolderIndex].append = 1;
}
/*
Another hour of hand-to-hand combat with pointers and this is still garbage
code
*/
void parseCommand() {
  int wordsToCheck = p->entries[p->commandCount - 1].argumentCount;
  commandHolder = malloc(sizeof(command) * maxCommandChain);
  commandHolderInit = 0;
  commandHolderEntriesUsed = 0;
  for (int i = 0; i < maxCommandChain; i++) {
    commandHolder[i].pipeTo = 0;
    commandHolder[i].argumentCount = 0;
    commandHolder[i].append = 0;
    commandHolder[i].runNext = 0;
    commandHolder[i].backgroundTask = 0;
    commandHolder[i].hasInput = 0;
    commandHolder[i].hasOutput = 0;
    commandHolder[i].arguments = malloc(sizeof(argument) * maxSubArgs);
    commandHolderInit++;
    // commandHolderEntriesUsed++;
  }
  char *wordBuffer[wordsToCheck];
  memset(&wordBuffer, 0, sizeof(wordBuffer));
  int bufPointer = 0;
  int currentCommand = 0;
  for (int i = 0; i < wordsToCheck; i++) {
    // printf("I: %d, words to check: %d\n", i, wordsToCheck);
    char *currentWord = p->entries[p->commandCount - 1].args[i];
    int wordLength = strlen(currentWord);
    if (wordLength >= 1) {
      switch (currentWord[0]) {
      case (60): // Represents <
        if (wordLength == 1 && i != wordsToCheck) {
          storeInput(currentCommand, i + 1);
          goto incrementPartial;
          break;
        } else {
          freeCommand(1);
          return;
        }
        break;
      case (62): // Represents >
        if (wordLength == 1 && i != wordsToCheck) {
          storeOutput(currentCommand, i + 1, 0);
          goto increment;
          break;
        } else if (wordLength == 2 && currentWord[1] == 62 &&
                   i != wordsToCheck) {
          storeOutput(currentCommand, i + 1, 1);
          goto increment;
          break;
        } else {
          freeCommand(1);
          return;
        }
        break;
      case (38): // Repesents &
        if (wordLength == 1) {
          commandHolder[currentCommand].backgroundTask = 1;
          break;
        } else if (wordLength == 2 && i != wordsToCheck &&
                   currentWord[1] == 38) {
          commandHolder[currentCommand].runNext = currentCommand + 1;
          goto increment;
          break;
        } else {
          freeCommand(1);
          return;
        }
        break;
      case (124): // Represents |
        if (wordLength > 1 || i == wordsToCheck) {
          freeCommand(1);
          return;
        } else {
          commandHolder[currentCommand].pipeTo = currentCommand + 1;
          goto incrementPartial;
          break;
        }
        break;
      case (-1):
      increment:
        i++;
      incrementPartial:
        currentCommand++;
        commandHolderEntriesUsed++;
        commandsToExecute++;
        bufPointer = 0;
        break;
      default:
        commandHolder[currentCommand].arguments[bufPointer] = strdup(currentWord);
        bufPointer++;
        commandHolder[currentCommand].argumentCount++;
        break;
      }
    }
  }
}

/*
This is old and just here for reference
*/
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