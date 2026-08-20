#include <klib.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

static int vsnprintf_internal(char *out, size_t n, const char *fmt, va_list ap) {
  char *dst = out;
  char *end = out + n - 1;

  while (*fmt && dst < end) {
    if (*fmt != '%') {
      *dst++ = *fmt++;
      continue;
    }

    fmt++; // skip '%'

    // Handle flags
    int zero_pad = 0;
    if (*fmt == '0') {
      zero_pad = 1;
      fmt++;
    }

    // Handle width
    int width = 0;
    while (*fmt >= '0' && *fmt <= '9') {
      width = width * 10 + (*fmt - '0');
      fmt++;
    }

    // Handle length modifier
    int is_long = 0;
    int is_long_long = 0;
    if (*fmt == 'l') {
      fmt++;
      is_long = 1;
      if (*fmt == 'l') {
        fmt++;
        is_long_long = 1;
      }
    }

    // Handle conversion specifier
    switch (*fmt) {
      case 'd':
      case 'i': {
        long long val;
        if (is_long_long) val = va_arg(ap, long long);
        else if (is_long) val = va_arg(ap, long);
        else val = va_arg(ap, int);

        if (val < 0) {
          if (dst < end) *dst++ = '-';
          val = -val;
        }

        char buf[32];
        int len = 0;
        if (val == 0) {
          buf[len++] = '0';
        } else {
          while (val > 0) {
            buf[len++] = '0' + (val % 10);
            val /= 10;
          }
        }

        // Padding
        while (len < width && dst < end) {
          *dst++ = zero_pad ? '0' : ' ';
          width--;
        }

        // Reverse output
        for (int i = len - 1; i >= 0 && dst < end; i--) {
          *dst++ = buf[i];
        }
        break;
      }

      case 'u': {
        unsigned long long val;
        if (is_long_long) val = va_arg(ap, unsigned long long);
        else if (is_long) val = va_arg(ap, unsigned long);
        else val = va_arg(ap, unsigned int);

        char buf[32];
        int len = 0;
        if (val == 0) {
          buf[len++] = '0';
        } else {
          while (val > 0) {
            buf[len++] = '0' + (val % 10);
            val /= 10;
          }
        }

        while (len < width && dst < end) {
          *dst++ = zero_pad ? '0' : ' ';
          width--;
        }

        for (int i = len - 1; i >= 0 && dst < end; i--) {
          *dst++ = buf[i];
        }
        break;
      }

      case 'x':
      case 'X': {
        unsigned long long val;
        if (is_long_long) val = va_arg(ap, unsigned long long);
        else if (is_long) val = va_arg(ap, unsigned long);
        else val = va_arg(ap, unsigned int);

        const char *digits = (*fmt == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";

        char buf[32];
        int len = 0;
        if (val == 0) {
          buf[len++] = '0';
        } else {
          while (val > 0) {
            buf[len++] = digits[val % 16];
            val /= 16;
          }
        }

        while (len < width && dst < end) {
          *dst++ = zero_pad ? '0' : ' ';
          width--;
        }

        for (int i = len - 1; i >= 0 && dst < end; i--) {
          *dst++ = buf[i];
        }
        break;
      }

      case 'p': {
        void *ptr = va_arg(ap, void *);
        unsigned long long val = (unsigned long long)(uintptr_t)ptr;

        if (dst + 2 < end) {
          *dst++ = '0';
          *dst++ = 'x';
        }

        char buf[32];
        int len = 0;
        if (val == 0) {
          buf[len++] = '0';
        } else {
          while (val > 0) {
            buf[len++] = "0123456789abcdef"[val % 16];
            val /= 16;
          }
        }

        for (int i = len - 1; i >= 0 && dst < end; i--) {
          *dst++ = buf[i];
        }
        break;
      }

      case 's': {
        const char *s = va_arg(ap, const char *);
        if (!s) s = "(null)";

        int len = 0;
        while (s[len] && len < width) len++;
        while (s[len]) len++;

        for (int i = 0; i < len && dst < end; i++) {
          *dst++ = s[i];
        }
        break;
      }

      case 'c': {
        int c = va_arg(ap, int);
        if (dst < end) *dst++ = c;
        break;
      }

      case '%':
        if (dst < end) *dst++ = '%';
        break;

      default:
        if (dst < end) *dst++ = '%';
        if (dst < end) *dst++ = *fmt;
        break;
    }

    fmt++;
  }

  *dst = '\0';
  return dst - out;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  if (n == 0) return 0;
  return vsnprintf_internal(out, n, fmt, ap);
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  return ret;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  return vsnprintf_internal(out, 0x7fffffff, fmt, ap);
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(out, fmt, ap);
  va_end(ap);
  return ret;
}

int vprintf(const char *fmt, va_list ap) {
  char buf[1024];
  int len = vsnprintf(buf, sizeof(buf), fmt, ap);
  for (int i = 0; i < len; i++) {
    putch(buf[i]);
  }
  return len;
}

int printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vprintf(fmt, ap);
  va_end(ap);
  return ret;
}

int __am_vsscanf_internal(const char *str, const char **end_pstr, const char *fmt, va_list ap) {
  const char *pstr = str;
  const char *pfmt = fmt;
  int item = -1;
  while (*pfmt) {
    char ch = *pfmt ++;
    if (isspace(ch)) {
      for (ch = *pfmt; isspace(ch); ch = *(++ pfmt));
      for (ch = *pstr; isspace(ch); ch = *(++ pstr));
      item ++;
      continue;
    }
    switch (ch) {
      case '%': break;
      default:
        if (*pstr == ch) { // match
          pstr ++;
          item ++;
          continue;
        }
        goto end; // fail
    }

    char *p;
    ch = *pfmt ++;
    switch (ch) {
      // conversion specifier
      case 'd':
        *(va_arg(ap, int *)) = strtol(pstr, &p, 10);
        if (p == pstr) goto end; // fail
        pstr = p;
        item ++;
        break;

      case 'c':
        *(va_arg(ap, char *)) = *pstr ++;
        item ++;
        break;

      default:
        printf("Unsupported conversion specifier '%c'\n", ch);
        assert(0);
    }
  }

end:
  if (end_pstr) {
    *end_pstr = pstr;
  }
  return item;
}

int vsscanf(const char *str, const char *fmt, va_list ap) {
  return __am_vsscanf_internal(str, NULL, fmt, ap);
}

int sscanf(const char *str, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsscanf(str, fmt, ap);
  va_end(ap);
  return r;
}

int __isoc99_sscanf(const char *str, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsscanf(str, fmt, ap);
  va_end(ap);
  return r;
}

#endif
