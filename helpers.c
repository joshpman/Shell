#include "shell.h"
#define shellHeader "[My-Shell] "
char *commands[] = {"cd"};
char **argumentListPtr = 0;
int argCount = 0;
int childPID;
void childSignalHandler(int signum) { 
  kill(childPID, SIGINT);
  if(argumentListPtr!=0){

  }
}
void freeArgumentList(){
  if(argumentListPtr==0) return;
  for(int i = 0; i<argCount; i++){
    free(argumentListPtr[i]);
  }
}
char *getHomeDirectory() {
  uid_t callingUserID = getuid();
  struct passwd *userPasswdFile = getpwuid(callingUserID);
  return userPasswdFile->pw_dir;
}
void autocomplete(char *readHere, int inputLength) {}
void executeCommand(char *argumentList[], int wordCount) {
  if (strcmp(argumentList[0], "cd") == 0) {
    changeDirectory(argumentList[1]);
  } else if (strcmp(argumentList[0], "quit") == 0) {
    cleanup(0);
  } else {
    argumentList[wordCount] = NULL;
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
      write(1, returnData, strlen(returnData));
      free(returnData);
      close(pipeFD[0]);
    } else {
      waitpid(child, NULL, 0);
    }
    freeArgumentList();
    argumentListPtr = 0;
    argCount = 0;
    signal(SIGINT, cleanup);
  }
}

  void writeHeader() { write(1, shellHeader, strlen(shellHeader)); }

  void processCommand(char *buffer, int bytesRead) {
    int foundSpace = 0;
    int wordCount = 0;
    char argumentBuffer[256];
    for (int i = 0; i < bytesRead; i++) {
      if (buffer[i] == 32) {
        foundSpace = 1;
      } else if (buffer[i] == 10) {
        wordCount++;
        break;
      } else if (foundSpace == 1) {
        wordCount++;
        foundSpace = 0;
      }
    }
    char *argumentList[wordCount + 1];
    int currentWord = 0;
    int currentChar = 0;
    char wordBuffer[64];
    foundSpace = 0;
    for (int i = 0; i < bytesRead; i++) {
      // Found a space
      if (buffer[i] == 32) {
        if (foundSpace == 0 &&
            currentChar != 0) { // If this isnt an empty word/series of spaces,
                                // store back the word
          wordBuffer[currentChar] = '\0';
          argumentList[currentWord] = malloc((currentChar + 1) * sizeof(char));
          currentChar = 0;
          strcpy(argumentList[currentWord++], wordBuffer);
          memset(wordBuffer, 0, sizeof(wordBuffer));
        }
        foundSpace = 1;
      } else if (buffer[i] == 10) { // Found a newline
        if (currentChar != 0) { // If this isnt an empty word/series of spaces,
                                // store back the word
          wordBuffer[currentChar] = '\0';
          argumentList[currentWord] = malloc((currentChar + 1) * sizeof(char));
          strcpy(argumentList[currentWord++], wordBuffer);

          currentChar = 0;
          memset(wordBuffer, 0, sizeof(wordBuffer));
        }
        break;
      } else {
        if (foundSpace == 1)
          foundSpace = 0;
        wordBuffer[currentChar++] = buffer[i];
      }
    }
    argumentListPtr = argumentList;
    argCount = wordCount;
    executeCommand(argumentList, wordCount);
  }

  void changeDirectory(char *newDirectory) {
    if (chdir(newDirectory) < 0) {
      write(2, "cd failed: bad path!\n", 22);
    }
  }

  void returnHome(char *homeDirectory) {
    printf("Home directory is %s\n", homeDirectory);
    if (chdir(homeDirectory) < 0) {
      write(2, "Setup failed!\n", 15);
      exit(1);
    }
  }
