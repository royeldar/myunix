#ifndef __LIMITS_H__
#define __LIMITS_H__

#define CHAR_BIT    8
#define SCHAR_MIN   (-SCHAR_MAX - 1)
#define SCHAR_MAX   (+(char)(UCHAR_MAX >> 1))
#define UCHAR_MAX   (+(unsigned char)-1)
#define MB_LEN_MAX  4
#define SHRT_MIN    (-SHRT_MAX - 1)
#define SHRT_MAX    (+(short)(USHRT_MAX >> 1))
#define USHRT_MAX   (+(unsigned short)-1)
#define INT_MIN     (-INT_MAX - 1)
#define INT_MAX     ((int)(UINT_MAX >> 1))
#define UINT_MAX    ((unsigned int)-1)
#define LONG_MIN    (-LONG_MAX - 1)
#define LONG_MAX    ((long)(ULONG_MAX >> 1))
#define ULONG_MAX   ((unsigned long)-1)
#define LLONG_MIN   (-LLONG_MAX - 1)
#define LLONG_MAX   ((long long)(ULLONG_MAX >> 1))
#define ULLONG_MAX  ((unsigned long long)-1)
#define CHAR_MIN    SCHAR_MIN
#define CHAR_MAX    SCHAR_MAX

#endif // __LIMITS_H__
