/* cs_util.c - see cs_util.h: preferences, progress, wrapping, KeyTips, small string helpers */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cs_util.h"
#include "cs_cmd.h"

char *cs_strlcpy(char *dst, const char *src, size_t cap)
{
  size_t n;
  if (!cap) return dst;
  n = src ? strlen(src) : 0;
  if (n >= cap) n = cap - 1;
  if (n) memcpy(dst, src, n);
  dst[n] = 0;
  return dst;
}
int cs_strieq(const char *a, const char *b)
{
  for (; *a && *b; a++, b++) if (cs_fold((unsigned char)*a) != cs_fold((unsigned char)*b)) return 0;
  return !*a && !*b;
}

/* ---------------------------------------------------------------- preferences */
static struct { char key[33]; char *val; } pref[CS_PREFS_MAX];
static int npref;

void cs_prefs_clear(void)
{
  int i;
  for (i = 0; i < npref; i++) free(pref[i].val);
  npref = 0;
}
static int find(const char *key)
{
  int i;
  for (i = 0; i < npref; i++) if (!strcmp(pref[i].key, key)) return i;
  return -1;
}
int cs_prefs_set(const char *key, const char *value)
{
  int i = find(key);
  char *v;
  size_t n = strlen(value);
  if (strlen(key) > 32 || strchr(key, '=') || strchr(key, '\n')) return 0;
  if (n > 255) n = 255;
  if (!(v = (char *)malloc(n + 1))) return 0;
  memcpy(v, value, n); v[n] = 0;
  for (n = 0; v[n]; n++) if (v[n] == '\n' || v[n] == '\r') v[n] = ' ';
  if (i < 0) {
    if (npref >= CS_PREFS_MAX) { free(v); return 0; }
    i = npref++;
    cs_strlcpy(pref[i].key, key, sizeof pref[i].key);
  } else free(pref[i].val);
  pref[i].val = v;
  return 1;
}
int cs_prefs_set_int(const char *key, long value)
{
  char b[24];
  sprintf(b, "%ld", value);
  return cs_prefs_set(key, b);
}
const char *cs_prefs_get(const char *key, const char *def)
{
  int i = find(key);
  return i < 0 ? def : pref[i].val;
}
long cs_prefs_int(const char *key, long def)
{
  const char *v = cs_prefs_get(key, NULL);
  char *end;
  long n;
  if (!v || !*v) return def;
  n = strtol(v, &end, 10);
  return *end ? def : n;
}
/* each "key=value" line; blank lines and "#" lines skipped; spaces round the key taken off */
int cs_prefs_load(const char *path)
{
  FILE *f = fopen(path, "r");
  char line[300];
  if (!f) return 0;
  while (fgets(line, sizeof line, f)) {
    char *eq = strchr(line, '='), *k = line, *e;
    size_t n = strlen(line);
    while (n && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = 0;
    if (!eq || line[0] == '#') continue;
    *eq = 0;
    while (*k == ' ') k++;
    for (e = eq; e > k && e[-1] == ' '; ) *--e = 0;
    if (*k) cs_prefs_set(k, eq + 1);
  }
  fclose(f);
  return 1;
}
int cs_prefs_save(const char *path)
{
  FILE *f = fopen(path, "w");
  int i, ok;
  if (!f) return 0;
  for (i = 0; i < npref; i++) fprintf(f, "%s=%s\n", pref[i].key, pref[i].val);
  ok = !ferror(f);
  if (fclose(f)) ok = 0;
  return ok;
}

/* ---------------------------------------------------------------- progress */
int (*cs_progress_hook)(long done, long total);
static long stage_lo = 0, stage_hi = 1000;
/* done/total within the stage, as thousandths of the whole */
int cs_progress(long done, long total)
{
  long t;
  if (!cs_progress_hook) return 0;
  if (total <= 0) total = 1;
  if (done > total) done = total;
  if (done < 0) done = 0;
  t = stage_lo + cs_muldiv(stage_hi - stage_lo, done, total);
  return cs_progress_hook(t, 1000);
}
long cs_progress_push(long lo, long hi)
{
  long saved = stage_lo * 2000 + stage_hi, span = stage_hi - stage_lo;
  long nlo = stage_lo + cs_muldiv(span, lo, 1000), nhi = stage_lo + cs_muldiv(span, hi, 1000);
  stage_lo = nlo; stage_hi = nhi;
  return saved;
}
void cs_progress_pop(long saved) { stage_lo = saved / 2000; stage_hi = saved % 2000; }

/* ---------------------------------------------------------------- wrapping */
char *cs_wrap(const char *in, char *out, size_t cap, int width)
{
  size_t o = 0;
  int col = 0;
  if (!cap) return out;
  while (*in && o + 1 < cap) {
    const char *sp = in;
    int wl;
    if (*in == '\n') { out[o++] = '\n'; col = 0; in++; continue; }
    if (*in == ' ') { in++; continue; }
    while (*sp && *sp != ' ' && *sp != '\n') sp++;
    wl = (int)(sp - in);
    if (col && col + 1 + wl > width) { out[o++] = '\n'; col = 0; }
    else if (col) { out[o++] = ' '; col++; }
    while (in < sp && o + 1 < cap) { out[o++] = *in++; col++; }
  }
  out[o] = 0;
  return out;
}

/* ---------------------------------------------------------------- KeyTips */
/* a character usable in a code: A-Z or 0-9 (accented letters as their plain one), else 0 */
static int tipch(int c)
{
  c = cs_fold(c);
  if (c >= 'a' && c <= 'z') return c - 32;
  if (c >= '0' && c <= '9') return c;
  return 0;
}
static int first_of(const char *s) { int f = 0; if (s) for (; *s && !(f = tipch((unsigned char)*s)); s++) ; return f; }
void cs_keytips_assign(const char *const *names, int n, char (*codes)[3])
{
  int i, j, k;
  for (i = 0; i < n; i++) codes[i][0] = 0;
  for (i = 0; i < n; i++) {
    int first = first_of(names[i]), count = 0;
    if (codes[i][0] || !first) continue;
    for (j = i; j < n; j++) if (first_of(names[j]) == first) count++;
    if (count == 1) { codes[i][0] = (char)first; codes[i][1] = 0; continue; }
    /* two characters: the first, then a later letter of the name's own that no other code with
       that first letter has, else a digit */
    for (j = i; j < n; j++) {
      const char *t = names[j], *q;
      int got = 0;
      if (!t || codes[j][0] || first_of(t) != first) continue;
      while (*t && !tipch((unsigned char)*t)) t++;
      for (q = t + 1; *q && !got; q++) {
        int c2 = tipch((unsigned char)*q), dup = 0;
        if (!c2) continue;
        for (k = 0; k < n; k++) if (codes[k][0] == first && codes[k][1] == c2) dup = 1;
        if (!dup) { codes[j][0] = (char)first; codes[j][1] = (char)c2; codes[j][2] = 0; got = 1; }
      }
      for (q = "123456789"; !got && *q; q++) {
        int dup = 0;
        for (k = 0; k < n; k++) if (codes[k][0] == first && codes[k][1] == *q) dup = 1;
        if (!dup) { codes[j][0] = (char)first; codes[j][1] = *q; codes[j][2] = 0; got = 1; }
      }
    }
  }
}
int cs_keytips_match(char (*codes)[3], int n, const char *typed)
{
  int i, part = 0;
  size_t tl = strlen(typed);
  for (i = 0; i < n; i++) {
    if (!codes[i][0]) continue;
    if (!strcmp(codes[i], typed)) return i;
    if (!strncmp(codes[i], typed, tl)) part = 1;
  }
  return part ? -1 : -2;
}

/* a*b/c in 32 bits where it fits, else split so nothing overflows (c > 0) */
long cs_muldiv(long a, long b, long c)
{
  int neg = 0;
  unsigned long ua, ub, uc, q, r;
  if (c == 0) return 0;
  if (a < 0) { a = -a; neg = !neg; }
  if (b < 0) { b = -b; neg = !neg; }
  if (c < 0) { c = -c; neg = !neg; }
  ua = (unsigned long)a; ub = (unsigned long)b; uc = (unsigned long)c;
  if (ub == 0 || ua <= 0x7FFFFFFFUL / ub) q = ua * ub / uc;
  else {
    /* (a*b)/c = (a/c)*b + ((a%c)*b)/c, the second part again in halves of b if need be */
    unsigned long qa = ua / uc, ra = ua % uc;
    q = qa * ub;
    r = 0;
    while (ub) {
      unsigned long part = ub > 0xFFFFUL ? 0xFFFFUL : ub;
      if (ra > 0 && part > 0x7FFFFFFFUL / ra) part = 0x7FFFFFFFUL / ra;
      if (!part) part = 1;
      r += ra * part / uc;
      ub -= part;
    }
    q += r;
  }
  return neg ? -(long)q : (long)q;
}
