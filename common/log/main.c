#include "log.h"

int main() {
    if (init_log()) {
        printf("Logger initialization failed.\n");
    }
    trace("Information: %d", 5);
    /*log_i("Information");
    log_d("Debugging");
    log_w("Warning");
    log_e("Error");
    log_f("Wasted");*/

    return 0;
}
