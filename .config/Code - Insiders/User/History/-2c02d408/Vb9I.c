#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main()
{
  srand(time(NULL));

  int number = (rand() % 100) + 1;

  printf("%d", number);

  return 0;
}