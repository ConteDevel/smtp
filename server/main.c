#include "pch.h"

int main() {
	if (init_log()) {
		printf("Can't initialize logger");
		return 1;
	}
    LOG_W("Hello, world!");

    return 0;
}
