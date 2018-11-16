#include <utils.h>

int main(int argc, char const *argv[])
{
    init_logs();

    LOG_I("Hello, world! %u", time(NULL));
    
    return 0;
}
