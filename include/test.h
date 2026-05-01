#ifndef CMONKEY_TEST_H
#define CMONKEY_TEST_H

#include "common_single.h"


typedef enum {
    // waiting to start test
    CMONKEY_WAITING     = 0,
    // test undergoing
    CMONKEY_UNDERGOING,
    // test finished, on result screen
    CMONKEY_FINISHED,
} CMONKEY_STATE;

typedef struct {
    u32 test_time;
    float elapsed_time;
} cmonkey_test;

#endif // CMONKEY_TEST_H
