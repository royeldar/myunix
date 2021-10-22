#ifndef __STDDEF_H__
#define __STDDEF_H__

#include <stdint.h>

typedef intptr_t        ptrdiff_t;
typedef unsigned long   size_t;
typedef int32_t         wchar_t

#define NULL            ((void *)0)
#define offsetof(t, m)  ((size_t)((char *)&((t *)NULL)->m - NULL))

#endif // __STDDEF_H__
