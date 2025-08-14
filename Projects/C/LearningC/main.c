#include <stdio.h>

int main() {
  FILE *pFile = fopen("file.txt", "r");

  char buffer[1024] = {0};

  if (pFile == NULL) {
    printf("Error while opening file");
    return 1;
  }

  while (fgets(buffer, sizeof(buffer), pFile) != NULL) {
    printf("%s", buffer);
  }

  fclose(pFile);

  return 0;
}
