#ifndef __FLOAT_H__
#define __FLOAT_H__

#define FLT_ROUNDS      -1  // TODO redefine to a predefined macro

#define FLT_EVAL_METHOD -1  // TODO redefine to a predefined macro

#define FLT_RADIX       2
#define FLT_MANT_DIG    24
#define DBL_MANT_DIG    53
#define LDBL_MANT_DIG   64
#define DECIMAL_DIG     21
#define FLT_DIG         6
#define DBL_DIG         15
#define LDBL_DIG        18
#define FLT_MIN_EXP     (-125)
#define DBL_MIN_EXP     (-1021)
#define LDBL_MIN_EXP    (-16381)
#define FLT_MIN_10_EXP  (-37)
#define DBL_MIN_10_EXP  (-307)
#define LDBL_MIN_10_EXP (-4931)
#define FLT_MAX_EXP     (+128)
#define DBL_MAX_EXP     (+1024)
#define LDBL_MAX_EXP    (+16384)
#define FLT_MAX_10_EXP  (+38)
#define DBL_MAX_10_EXP  (+308)
#define LDBL_MAX_10_EXP (+4932)
#define FLT_MAX         0x1.fffffeP+127F
#define DBL_MAX         0x1.fffffffffffffP+1023
#define LDBL_MAX        0x1.fffffffffffffffeP+16383L
#define FLT_EPSILON     0x1P-23F
#define DBL_EPSILON     0x1P-52
#define LDBL_EPSILON    0x1P-63L
#define FLT_MIN         0x1P-126F
#define DBL_MIN         0x1P-1022
#define LDBL_MIN        0x1P-16382L

#endif // __FLOAT_H__
