#include <stdio.h>

int main()
{
  FILE *pFile = fopen("file.txt", "w");

  char text[] = "Im am a test file heloooo!!!!";

  if (pFile == NULL)
  {
    printf("Error while opening file");
    return 1;
  }

  fprintf(pFile, "%s", text);

  fclose(pFile);

  return 0;
}