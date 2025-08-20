#include <stdio.h>

int main()
{
  FILE *pFile = fopen("file.txt", "w");

  if (pFile == NULL)
  {
    printf("Error while opening file");
    return 1;
  }

  fclose(pFile);

  return 0;
}