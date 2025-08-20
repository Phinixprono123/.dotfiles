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

  printf("GUESSS THE NUMBER\n");
  do
  {
    printf("guess a number between %d and %d: ", min, max);
    scanf("%d", &guess);
    tries++;

    if (guess < answer)
    {
      printf("Too low\n");
    }
    else if (guess > answer)
    {
      printf("Too high\n");
    }

  } while (guess != answer);

  printf("\nThe answer is %d \n", answer);
  printf("It took you %d tries to guess it\n", tries);

  return 0;
}