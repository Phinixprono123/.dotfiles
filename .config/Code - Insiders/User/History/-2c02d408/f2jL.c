#include <stdio.h>
#include <math.h>

int main()
{
   char operartor = '\0';
   double num1 = 0.0;
   double num2 = 0.0;
   double result = 0.0;

   printf("Enter the first number: ");
   scanf("%lf", &num1);

   printf("Enter an operator (+ - / *): ");
   scanf(" %c", &operartor);

   printf("Enter the second number: ");
   scanf(" 
%lf", &num2);

   return 0;
}