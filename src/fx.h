#ifndef FX_H
#define FX_H
#include <stdint.h>
typedef int32_t fx;
#define FX_ONE 256
#define FX(i) ((fx)(i) * FX_ONE)
/* callers keep operands small enough that the product fits int32 */
#define FXMUL(a,b) ((fx)(((int32_t)(a) * (b)) >> 8))
#endif
