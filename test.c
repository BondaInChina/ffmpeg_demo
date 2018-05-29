#include "random.h"
static int my_rand(char *rand,int len)
{
memset(rand,0x02,len);
return 0;
}
int main()
{
char random[10];
int ret;
set_callback(my_rand);
ret=genrate_random(random,10);
return 0;
}
