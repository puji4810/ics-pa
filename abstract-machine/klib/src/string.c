#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  if(s == NULL) 
    return 0;
  else {
    size_t len = 0;
    while(s[len] != '\0') len++;
    return len;
  }
}

char *strcpy(char *dst, const char *src) {
  if(src == NULL) return NULL;
  else {
    char *ret = dst;
    while((*dst++ = *src++) != '\0');
    return ret;
  }
}

char *strncpy(char *dst, const char *src, size_t n) {
  if (src == NULL)
    return NULL;
  else {
    while(*src != '\0') 
    {
      if(n-- == 0 ) break;
      *dst = *src;
      dst++;
      src++;
    }
    *dst = '\0';
    return dst;
  }
}

char *strcat(char *dst, const char *src) {
  if(src == NULL) return dst;
  else {
    char *ret = dst;
    while(*dst != '\0') dst++;
    while((*dst++ = *src++) != '\0');
    return ret;
  }
}

int strcmp(const char *s1, const char *s2) {
  if(s1 == NULL && s2 == NULL) return 0;
  else if(s1 == NULL) return -1;
  else if(s2 == NULL) return 1;
  else {
    while(*s1 != '\0' && *s2 != '\0') {
      if(*s1 < *s2) return -1;
      else if(*s1 > *s2) return 1;
      s1++;
      s2++;
    }
    if(*s1 == '\0' && *s2 == '\0') return 0;
    else if(*s1 == '\0') return -1;
    else return 1;
  }
}

int strncmp(const char *s1, const char *s2, size_t n) {
  if(s1 == NULL && s2 == NULL) return 0;
  else if(s1 == NULL) return -1;
  else if(s2 == NULL) return 1;
  else {
    while(*s1 != '\0' && *s2 != '\0' && n-- > 0) {
      if(*s1 < *s2) return -1;
      else if(*s1 > *s2) return 1;
      s1++;
      s2++;
    }
    if(n == 0) return 0;
    if(*s1 == '\0' && *s2 == '\0') return 0;
    else if(*s1 == '\0') return -1;
    else return 1;
  }
}

void *memset(void *s, int c, size_t n) {
  char *p = (char *)s;
  while(n-- > 0) *p++ = c;
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  char *d = (char *)dst;
  const char *s = (const char *)src;
  if(d < s) {
    while(n-- > 0) *d++ = *s++;
  }
  else {
    d += n;
    s += n;
    while(n-- > 0) *--d = *--s;
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  char *d = (char *)out;
  const char *s = (const char *)in;
  while(n-- > 0) *d++ = *s++;
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = (const unsigned char *)s1;
  const unsigned char *p2 = (const unsigned char *)s2;
  while(n-- > 0) {
    if(*p1 < *p2) return -1;
    else if(*p1 > *p2) return 1;
    p1++;
    p2++;
  }
  return 0;
}

#endif
