#ifndef __STDINT_H__
#define __STDINT_H__

#include <limits.h>

typedef signed char         int8_t;
typedef unsigned char       uint8_t;
typedef short               int16_t;
typedef unsigned short      uint16_t;
typedef int                 int32_t;
typedef unsigned int        uint32_t;
typedef long long           int64_t;
typedef unsigned long long  uint64_t;

typedef int8_t              int_least8_t;
typedef uint8_t             uint_least8_t;
typedef int16_t             int_least16_t;
typedef uint16_t            uint_least16_t;
typedef int32_t             int_least32_t;
typedef uint32_t            uint_least32_t;
typedef int64_t             int_least64_t;
typedef uint64_t            uint_least64_t;

typedef int8_t              int_fast8_t;
typedef uint8_t             uint_fast8_t;
typedef int16_t             int_fast16_t;
typedef uint16_t            uint_fast16_t;
typedef int32_t             int_fast32_t;
typedef uint32_t            uint_fast32_t;
typedef int64_t             int_fast64_t;
typedef uint64_t            uint_fast64_t;

typedef long                intptr_t;
typedef unsigned long       uintptr_t;

typedef int64_t             intmax_t;
typedef uint64_t            uintmax_t;

#define INT8_MIN            (-INT8_MAX - 1)
#define INT8_MAX            (+(int8_t)(UINT8_MAX >> 1))
#define UINT8_MAX           (+(uint8_t)-1)
#define INT16_MIN           (-INT16_MAX - 1)
#define INT16_MAX           (+(int16_t)(UINT16_MAX >> 1))
#define UINT16_MAX          (+(uint16_t)-1)
#define INT32_MIN           (-INT32_MAX - 1)
#define INT32_MAX           (+(int32_t)(UINT32_MAX >> 1))
#define UINT32_MAX          (+(uint32_t)-1)
#define INT64_MIN           (-INT64_MAX - 1)
#define INT64_MAX           (+(int64_t)(UINT64_MAX >> 1))
#define UINT64_MAX          (+(uint64_t)-1)

#define INT_LEAST8_MIN      (-INT_LEAST8_MAX - 1)
#define INT_LEAST8_MAX      (+(int_least8_t)(UINT_LEAST8_MAX >> 1))
#define UINT_LEAST8_MAX     (+(uint_least8_t)-1)
#define INT_LEAST16_MIN     (-INT_LEAST16_MAX - 1)
#define INT_LEAST16_MAX     (+(int_least16_t)(UINT_LEAST16_MAX >> 1))
#define UINT_LEAST16_MAX    (+(uint_least16_t)-1)
#define INT_LEAST32_MIN     (-INT_LEAST32_MAX - 1)
#define INT_LEAST32_MAX     (+(int_least32_t)(UINT_LEAST32_MAX >> 1))
#define UINT_LEAST32_MAX    (+(uint_least32_t)-1)
#define INT_LEAST64_MIN     (-INT_LEAST64_MAX - 1)
#define INT_LEAST64_MAX     (+(int_least64_t)(UINT_LEAST64_MAX >> 1))
#define UINT_LEAST64_MAX    (+(uint_least64_t)-1)

#define INT_FAST8_MIN       (-INT_FAST8_MAX - 1)
#define INT_FAST8_MAX       (+(int_fast8_t)(UINT_FAST8_MAX >> 1))
#define UINT_FAST8_MAX      (+(uint_fast8_t)-1)
#define INT_FAST16_MIN      (-INT_FAST16_MAX - 1)
#define INT_FAST16_MAX      (+(int_fast16_t)(UINT_FAST16_MAX >> 1))
#define UINT_FAST16_MAX     (+(uint_fast16_t)-1)
#define INT_FAST32_MIN      (-INT_FAST32_MAX - 1)
#define INT_FAST32_MAX      (+(int_fast32_t)(UINT_FAST32_MAX >> 1))
#define UINT_FAST32_MAX     (+(uint_fast32_t)-1)
#define INT_FAST64_MIN      (-INT_FAST64_MAX - 1)
#define INT_FAST64_MAX      (+(int_fast64_t)(UINT_FAST64_MAX >> 1))
#define UINT_FAST64_MAX     (+(uint_fast64_t)-1)

#define INTPTR_MIN          (-INTPTR_MAX - 1)
#define INTPTR_MAX          (+(intptr_t)(UINTPTR_MAX >> 1))
#define UINTPTR_MAX         (+(uintptr_t)-1)

#define INTMAX_MIN          (-INTMAX_MAX - 1)
#define INTMAX_MAX          ((intmax_t)(UINTMAX_MAX >> 1))
#define UINTMAX_MAX         ((uintmax_t)-1)

// ptrdiff_t is the same type as intptr_t; see <stddef.h>
#define PTRDIFF_MIN         (-PTRDIFF_MAX - 1)
#define PTRDIFF_MAX         INTPTR_MAX

// sig_atomic_t is the same type as int; see <signal.h>
#define SIG_ATOMIC_MIN      (-SIG_ATOMIC_MAX - 1)
#define SIG_ATOMIC_MAX      INT_MAX

// size_t is the same type as unsigned long; see <stddef.h>
#define SIZE_MAX            ULONG_MAX

// wchar_t is the same type as int32_t; see <stddef.h>
#define WCHAR_MIN           (-WCHAR_MAX - 1)
#define WCHAR_MAX           INT32_MAX

// wint_t is the same type as int32_t; see <wchar.h>
#define WINT_MIN            (-WINT_MAX - 1)
#define WINT_MAX            INT32_MAX

#define INT8_C(value)       value
#define UINT8_C(value)      value
#define INT16_C(value)      value
#define UINT16_C(value)     value
#define INT32_C(value)      value
#define UINT32_C(value)     value ## U
#define INT64_C(value)      value ## LL
#define UINT64_C(value)     value ## ULL

#define INTMAX_C(value)     INT64_C(value)
#define UINTMAX_C(value)    UINT64_C(value)

#endif // __STDINT_H__
