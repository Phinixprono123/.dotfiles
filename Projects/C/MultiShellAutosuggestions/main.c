#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define MAX_INPUT 1024

void enable_raw_mode(struct termios *orig_termios) {
  struct termios raw = *orig_termios;
  raw.c_lflag &= ~(ECHO | ICANON); // Disable echo and canonical mode
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disable_raw_mode(struct termios *orig_termios) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, orig_termios);
}

char *get_suggestion(const char *input) {
  HIST_ENTRY **history = history_list();
  if (!history)
    return NULL;

  for (int i = history_length - 1; i >= 0; i--) {
    if (strncmp(history[i]->line, input, strlen(input)) == 0) {
      return history[i]->line; // Return closest match
    }
  }
  return NULL;
}

int main() {
  struct termios orig_termios;
  tcgetattr(STDIN_FILENO, &orig_termios);
  enable_raw_mode(&orig_termios);

  char input[MAX_INPUT] = {0};
  int index = 0;

  printf("$ "); // Display prompt
  while (1) {
    char c = getchar();

    if (c == '\n') { // Enter key
      disable_raw_mode(&orig_termios);
      printf("\nRunning: %s\n", input);
      system(input);
      memset(input, 0, MAX_INPUT);
      index = 0;
      enable_raw_mode(&orig_termios);
      printf("$ ");
    } else if (c == '\t') { // Tab key for autocompletion
      char *suggestion = get_suggestion(input);
      if (suggestion) {
        strcpy(input, suggestion);
        index = strlen(input);
        printf("\r$ %s", input);
        fflush(stdout);
      }
    } else if (c == 127) { // Backspace
      if (index > 0) {
        input[--index] = '\0';
        printf("\r$ %s ", input);
        fflush(stdout);
      }
    } else {
      input[index++] = c;
      printf("%c", c);
      fflush(stdout);
    }
  }

  return 0;
}
