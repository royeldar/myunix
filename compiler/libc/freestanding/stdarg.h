#ifndef __STDARG_H__
#define __STDARG_H__

// TODO implement varargs

typedef __builtin_va_list   va_list;

// type va_arg(va_list ap, type);
#define va_arg(ap, type)    __builtin_va_arg(ap, type)
// void va_copy(va_list dest, va_list src);
#define va_copy(dest, src)  __builtin_va_copy(dest, src)
// void va_end(va_list ap);
#define va_end(ap)          __builtin_va_end(ap)
// void va_start(va_list ap, parmN);
#define va_start(ap, parmN) __builtin_va_start(ap, parmN)

#endif // __STDARG_H__
