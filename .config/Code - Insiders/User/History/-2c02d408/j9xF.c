#include <stdio.h>
#include <math.h>

int main()
{
   int x = 5; // 101 in binary
   int y = 3; // 011 in binary

   // Logical AND
   if (x > 0 && y > 0)
   {
      printf("Both x and y are positive\n");
   }

   // Bitwise AND
   int result = x & y; // 001 in binary = 1 in decimal
   printf("Bitwise AND result: %d\n", result);

   return 0;
}