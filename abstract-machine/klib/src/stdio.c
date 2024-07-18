#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int vprintf(const char *fmt, va_list ap){
  char buf[1024];
  vsprintf(buf, fmt, ap);
  int i = 0;
  while(buf[i] != '\0'){
    putch(buf[i]);
    i++;
  }
  return i;
}

int printf(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  int ret = vprintf(fmt, ap);
  va_end(ap);
  return ret;
}

int vsprintf(char *out, const char *fmt, va_list ap)
{
  char *p = out;
  const char *f = fmt;
  while (*f)
  {
    if (*f != '%')
    {
      *p++ = *f++;
      continue;
    }
    f++;
    switch (*f)
    {
    case 's':
    {
      const char *s = va_arg(ap, const char *);
      while (*s)
      {
        *p++ = *s++;
      }
      break;
    }
    case 'd':
    {
      int d = va_arg(ap, int);
      if (d < 0)
      {
        *p++ = '-';
        d = -d;
      }
      char buf[32];
      int i = 0;
      if (d == 0)
      {
        buf[i++] = '0';
      }
      while (d)
      {
        buf[i++] = d % 10 + '0';
        d /= 10;
      }
      while (i--)
      {
        *p++ = buf[i];
      }
      break;
    }
    case 'x':
    {
      unsigned int x = va_arg(ap, unsigned int);
      char buf[32];
      int i = 0;
      if (x == 0)
      {
        buf[i++] = '0';
      }
      while (x)
      {
        int t = x % 16;
        if (t < 10)
        {
          buf[i++] = t + '0';
        }
        else
        {
          buf[i++] = t - 10 + 'a';
        }
        x /= 16;
      }
      while (i--)
      {
        *p++ = buf[i];
      }
      break;
    }
    default:
      //panic("Not implemented");
    }
    f++;
  }
  *p = '\0';
  return p - out;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(out, fmt, ap);
  va_end(ap);
  return ret;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  int count = 0;  // count the number of characters printed
  char *p = out;
  const char *f = fmt;
  while (*f && count<n-1)
  {
    if (*f != '%')
    {
      *p++ = *f++;
      continue;
    }
    f++;
    switch (*f)
    {
    case 's':
    {
      const char *s = va_arg(ap, const char *);
      while (*s)
      {
        *p++ = *s++;
      }
      break;
    }
    case 'd':
    {
      int d = va_arg(ap, int);
      if (d < 0)
      {
        *p++ = '-';
        d = -d;
      }
      char buf[32];
      int i = 0;
      if (d == 0)
      {
        buf[i++] = '0';
      }
      while (d)
      {
        buf[i++] = d % 10 + '0';
        d /= 10;
      }
      while (i--)
      {
        *p++ = buf[i];
      }
      break;
    }
    case 'x':
    {
      unsigned int x = va_arg(ap, unsigned int);
      char buf[32];
      int i = 0;
      if (x == 0)
      {
        buf[i++] = '0';
      }
      while (x)
      {
        int t = x % 16;
        if (t < 10)
        {
          buf[i++] = t + '0';
        }
        else
        {
          buf[i++] = t - 10 + 'a';
        }
        x /= 16;
      }
      while (i--)
      {
        *p++ = buf[i];
      }
      break;
    }
    default:
      panic("Not implemented");
    }
    f++;
    count++;
  }
  *p = '\0';
  return p - out;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  return ret;
}



#endif
