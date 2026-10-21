#ifndef TEST_H
#define TEST_H
#include <stdio.h>
extern int g_fail;
#define CHECK(c) do { if (!(c)) { printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); g_fail++; } } while (0)
#endif
