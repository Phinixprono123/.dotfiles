#include <stdio.h>
#include <unistd.h>

int main()
{
  for (int i = 10; i >= 0; i--)
  {
    sleep(0.9);
    printf("%d\n", i);
  }

  printf("Happy new year!");

  return 0;
}