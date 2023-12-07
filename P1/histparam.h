#include <stdbool.h>

struct msg {
	int intervalCount;
    int intervalWidth;
    int intervalStart;
    bool termination;
};

#define MQNAME "/msgqueue"