#include "shell.h"
#include <fcntl.h>
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
void executeCommand() {
  // printf("Command list has %d entries\n", commandHolderEntriesUsed);
  // printf("This function has %d for pipeTo\n", currentEntry->pipeTo);
  // printf("This function has %s for outputFile\n", currentEntry->outputFile);
  command *currentEntry = commandHolder[0];
  argument *arguments = currentEntry->arguments;
  if (strcmp(arguments[0], "cd") == 0) {
    changeDirectory(arguments[1]);
    freeCommand(0);
    return;
  }
  for (int i = 0; i < currentEntry->argumentCount; i++) {
    printf("Current entry has %s at %d\n", currentEntry->arguments[i], i);
  };
  int pipeFD[2], previousPipeFD[2];
  int outputFD = 1;
  int inputFD = 0;
  int i = 0;
  int first = 1;
  //1st do while loop I've ever wrote
  do {
    printf("iteration!\n");
    currentEntry = commandHolder[i];

    //If we need to i/o redirect && not pipe(Which shouldn't be valid anyway)
    if (currentEntry->hasOutput && currentEntry->pipeTo <= 0) {
      //Check if we need to append i.e >> passed ijn
      if (currentEntry->append) {
        outputFD =
            open(currentEntry->outputFile, O_CREAT | O_APPEND | O_WRONLY, 0644);
      } else {
        outputFD =
            open(currentEntry->outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0644);
      }
    }

    //If this is the first task and it has an input file
    if (first && currentEntry->hasInput) {
      inputFD = open(currentEntry->inputFile, O_RDONLY);
      if(inputFD<=0){
        inputFD = -1;
        }//If it broke, reset it to -1
    }

    //If it has to pipe to a task
    if (currentEntry->pipeTo > 0) {
      pipe(pipeFD);
    }

    //Fork into child
    if ((childPID = fork()) == 0) {
      printf("Made it into child!\n");
      //If its the first child with 
      if (first) {
        if (inputFD != 1) {//If input fd didn't fail
          dup2(inputFD, 0);
          close(inputFD);
        }
      } else if(!first) {
        //Copy old pipes output for standard of this chilsd
        dup2(previousPipeFD[0], 0);
        close(previousPipeFD[0]);//Cleanup
        // close(previousPipeFD[1]);//Cleanup
      }

      //If we have to pipe again
      if (currentEntry->pipeTo > 0 && currentEntry->hasOutput) {
        dup2(outputFD, 1);//Override our standard in with write end of pipe
        close(outputFD);
      } else if (currentEntry->pipeTo>0) {
        //Else I/O Redirect
        dup2(outputFD, 1);
        close(pipeFD[1]);
      }

      char **execArgs =
          malloc(sizeof(char *) * (currentEntry->argumentCount + 1));
      for (int i = 0; i < currentEntry->argumentCount; i++) {
        execArgs[i] = currentEntry->arguments[i];
      }
      execArgs[currentEntry->argumentCount] = NULL;
            // printf("Made it into child!\n");
      if (execvp(execArgs[0], execArgs) < 0) {
        write(2, "Exec failed!\n", 14);
        free(execArgs);
        exit(-1);
      }
      free(execArgs);
      close(pipeFD[0]);
      close(pipeFD[1]);
      exit(0);
    } else {
      if (!first) {
        close(previousPipeFD[0]);
        close(previousPipeFD[1]);
      }
      if (currentEntry->pipeTo > 0) {
        previousPipeFD[0] = pipeFD[0];
        previousPipeFD[1] = pipeFD[1];
      }
    }
    wait(NULL);
    printf("Made it past child\n");
    first = 0;
    i++;
  } while (i < commandHolderEntriesUsed+1);

  freeCommand(-1);
}
void childSignalHandler(int signum) { kill(childPID, SIGINT); }

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
  if(commandHolderInit!=-1) freeCommand(-1);
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

//Why is this a function
void writeHeader() { write(1, shellHeader, strlen(shellHeader)); }


//Nice logic with so much pain in each of these functions
void processCommand(char *buffer, int bytesRead) {
  buildArgs(buffer, bytesRead);
  parseCommand();
  executeCommand();
}


//Weird stack thingy using memmove
void pushEntry(input *i) {
  if (p->commandCount == p->maxCommands) {
    memmove(&p->entries[1], &p->entries[0],
            (historySize - 1) * sizeof(input *));
  }
  p->entries[p->commandCount] = *i;

  //Ternary hell
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

//Parse input by splitting arguments with spaces/newlines between them and storing them in an input struct
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

/*
American Function(cuz its the land of the free)
*/
void freeCommand(int wasError) {
  printf("Command holder entries used is %d\n", commandHolderEntriesUsed);
  if (commandHolderInit == -1) {
    return;
  }
  for (int i = 0; i <= commandHolderEntriesUsed; i++) {
    printf("On iteration %d\n", i);
    for (int j = 0; j < commandHolder[i]->argumentCount; j++) {
      free(commandHolder[i]->arguments[j]);
    }
    free(commandHolder[i]->arguments);
    if (commandHolder[i]->hasOutput)
      free(commandHolder[i]->outputFile);
    if (commandHolder[i]->hasInput)
      free(commandHolder[i]->inputFile);
  }
  for (int i = 0; i < maxCommandChain; i++) {
    free(commandHolder[i]);
  }
  free(commandHolder);
  commandHolderInit = -1;
  commandHolderEntriesUsed = 0;
  if (wasError == 1) {
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
  for (int i = 0; i < maxCommandChain; i++) {
    commandHolder[i] = malloc(sizeof(command));
    commandHolder[i]->pipeTo = 0;
    commandHolder[i]->argumentCount = 0;
    commandHolder[i]->append = 0;
    commandHolder[i]->runNext = 0;
    commandHolder[i]->backgroundTask = 0;
    commandHolder[i]->hasInput = 0;
    commandHolder[i]->hasOutput = 0;
    commandHolder[i]->arguments = malloc(sizeof(argument *) * maxSubArgs);
    commandHolderInit++;
  }
  char *wordBuffer[wordsToCheck];
  memset(&wordBuffer, 0, sizeof(wordBuffer));

  int bufPointer = 0;
  int currentCommand = 0;
  commandHolderEntriesUsed = 0;
  for (int i = 0; i < wordsToCheck; i++) {
    // printf("I: %d, words to check: %d\n", i, wordsToCheck);
    char *currentWord = p->entries[p->commandCount - 1].args[i];
    int wordLength = strlen(currentWord);
    if (wordLength >= 1) {
      switch (currentWord[0]) {
      case (60): // Represents <
        if (wordLength == 1 && i != wordsToCheck) {
          commandHolder[currentCommand]->inputFile =
              malloc(sizeof(char) *
                     strlen(p->entries[p->commandCount - 1].args[i + 1]));
          strcpy(commandHolder[currentCommand]->inputFile, p->entries[p->commandCount - 1].args[i + 1]);
          commandHolder[currentCommand]->hasInput = 1;
          currentCommand++;
          commandHolderEntriesUsed++;
          i++;
          bufPointer = 0;
          break;
        } else {
          freeCommand(1);
          return;
        }
        break;
      case (62): // Represents >
        if (wordLength == 1 && i != wordsToCheck) {
          commandHolder[currentCommand]->outputFile =
              malloc(sizeof(char) *
                     strlen(p->entries[p->commandCount - 1].args[i + 1]));
          commandHolder[currentCommand]->outputFile =
              p->entries[p->commandCount - 1].args[i + 1];
          commandHolder[currentCommand]->hasOutput = 1;
          currentCommand++;
          bufPointer = 0;
          commandHolderEntriesUsed++;
          i++;
          break;
        } else if (wordLength == 2 && currentWord[1] == 62 &&
                   i != wordsToCheck) {
          commandHolder[currentCommand]->outputFile =
              malloc(sizeof(char) *
                     strlen(p->entries[p->commandCount - 1].args[i + 1]));
          commandHolder[currentCommand]->outputFile =
              p->entries[p->commandCount - 1].args[i + 1];
          commandHolder[currentCommand]->append = 1;
          commandHolder[currentCommand]->hasOutput = 1;
          currentCommand++;
          commandHolderEntriesUsed++;
          i++;
          bufPointer = 0;
          break;
        } else {
          freeCommand(1);
          return;
        }
        break;
      case (38): // Repesents &
        if (wordLength == 1) {
          commandHolder[currentCommand]->backgroundTask = 1;
          break;
        } else if (wordLength == 2 && i != wordsToCheck) {
          commandHolder[currentCommand]->runNext = currentCommand + 1;
          currentCommand++;
          commandHolderEntriesUsed++;
          bufPointer = 0;
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
          commandHolder[currentCommand]->pipeTo = currentCommand + 1;
          currentCommand++;
          commandHolderEntriesUsed++;
          bufPointer = 0;
          break;
        }
        break;
      default:
        commandHolder[currentCommand]->arguments[bufPointer] = malloc(
            sizeof(char) *
            (wordLength + 1)); // Allocate memory for string + null terminator
strncpy(commandHolder[currentCommand]->arguments[bufPointer], currentWord, wordLength + 1);
        bufPointer++;
        commandHolder[currentCommand]->argumentCount++;
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