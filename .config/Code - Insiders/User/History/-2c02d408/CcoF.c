#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main()
{
  srand(time(NULL));

  int guess = 0;
  int tries = 0;
  int min = 1;
  int max = 100;
  int answer = (rand() % (max - min + 1)) + min;

  printf("%d", answer);

  return 0;
}