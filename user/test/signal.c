#include "user.h"
static void my_sigint_handler(int)
{
    
}

int main()
{
    sigaction(2, my_sigint_handler);
    while (1) {
        sleep(500);
    }
}
