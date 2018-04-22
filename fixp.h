#ifndef ARCA_FIXP_H
#define ARCA_FIXP_H

/* 计算定点数的工具. */

/* 我们实现了一个简单的定点数类型，在小数点后有固定数目的小数位数。 */

#include <stdint.h>

typedef uint_fast32_t fixp_t;

/* 在整个数字部分，我们应该至少容纳0到131072(17位);假设至少有32位整数，这就留给我们15位小数部分。幸运的是，我们只需要无符号的值。 */
#define FIXP_BITS 15

#define FIXP_SCALE (1<<FIXP_BITS)

#define double_to_fixp(n) ((fixp_t) ((n) * (FIXP_SCALE)))
#define fixp_to_double(n) ((double) (n) / FIXP_SCALE)

#endif
