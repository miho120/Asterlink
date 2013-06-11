#include <stdio.h>
const char * getString();
int main()
{
 printf("hello world\n");
 printf("%s\n", getString());
 return 0;
}

const char * getString()
{
 const char *x = "abcstring";
 return x;
}
