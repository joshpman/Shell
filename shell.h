#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <termios.h>
#include <signal.h>
char* getHomeDirectory();
void returnHome(char* homeDirectory);
void writeHeader();
void processCommand(char* buffer, int bytesRead);
void changeDirectory(char* newDirectory);