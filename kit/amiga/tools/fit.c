/*
 * fit.c - the native harness behind ribbonfit.py and langfit.py: the program's own ribbon and menu
 * tables (its ui/ files, compiled natively), laid out by the kit's own layout code (cs_ribbon.c),
 * measured with the real screen font's widths (fitfont.h, written by the Python script from
 * QuillSans/11), in each of the five languages. Because it runs the same code the Amiga runs, the
 * check cannot drift from the class the way a model of it would.
 *
 * Built by the scripts with -DFIT_INIT=... -DFIT_RIBBON=... -DFIT_MENUS=... -DFIT_TABLE="..."
 *   fit ribbon WIDTH   every tab at WIDTH pixels: overflow, squeezing, groups collapsed, tab names cut
 *   fit menus          every menu's title bar and items in topaz 8 against a 640-pixel screen
 * Exit status 1 if anything doesn't fit. SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cs_cmd.h"
#include "cs_lang.h"
#include "cs_ribbon.h"
#include "fitfont.h"                          /* fit_adv[256], fit_height: QuillSans 11's advances */

void FIT_INIT(void);
#ifdef FIT_RIBBON
extern const CsRibbonDef FIT_RIBBON;
#endif
#ifdef FIT_MENUS
extern const CsMenuItem FIT_MENUS[];
#endif
static const CsWords words[] = {
#include FIT_TABLE
  { { NULL } }
};
static long width(void *c, const char *s)
{
  long w = 0;
  (void)c;
  for (; *s; s++) w += fit_adv[(unsigned char)*s];
  return w;
}
static const char *const codes[] = { "en", "de", "fr", "es", "it" };

#ifdef FIT_RIBBON
static int ribbon(int w)
{
  CsRbLayout *l = (CsRbLayout *)malloc(sizeof *l);
  CsRbMetrics m;
  int lang, t, g, bad = 0;
  m.fh = fit_height; m.textw = width; m.ctx = NULL;
  for (lang = 0; lang < CS_NLANG; lang++) {
    cs_lang_set(lang);
    for (t = 0; t < FIT_RIBBON.ntabs; t++) {
      int ncol = 0;
      char cut[200] = "";
      cs_rb_layout(&FIT_RIBBON, t, w, 0, 0, &m, l);
      if (l->tab != t) continue;                       /* (a contextual tab not shown now) */
      for (g = 0; g < l->ngroup; g++) if (l->collapsed[g]) {
        size_t n = strlen(cut);
        ncol++;
        snprintf(cut + n, sizeof cut - n, "%s%s", n ? ", " : "", tr(FIT_RIBBON.tabs[t].groups[g].name));
      }
      printf("%s %-10s %s%s%s%s\n", codes[lang], tr(FIT_RIBBON.tabs[t].name),
             l->overflow ? "DOES NOT FIT" : "fits", l->squeeze ? ", squeezed" : "",
             ncol ? ", collapsed: " : "", cut);
      if (l->overflow) { printf("   %d pixels too wide\n", l->overflow); bad++; }
    }
    if (l->tabcap) printf("%s tab names cut to %d pixels\n", codes[lang], l->tabcap);
  }
  free(l);
  return bad;
}
#endif
#ifdef FIT_MENUS
/* Intuition's menus in the screen's font (topaz 8, 8 pixels a letter, the widest usual case): the
   titles side by side across the screen's bar, each menu's items under it with room for the
   Amiga-key sign and letter, a submenu beside its item */
static int menus(void)
{
  int lang, i, bad = 0;
  for (lang = 0; lang < CS_NLANG; lang++) {
    long x = 4, itemw = 0, subw = 0, title_x = 0, worst = 0;
    const char *title = "";
    cs_lang_set(lang);
    for (i = 0; ; i++) {
      const CsMenuItem *it = &FIT_MENUS[i];
      if (it->kind == CSM_TITLE || it->kind == CSM_END) {
        long right = title_x + itemw + (subw ? subw - 16 : 0);
        if (i && right > 640) { printf("%s menu %s runs %ld pixels past a 640-pixel screen\n", codes[lang], tr(title), right - 640); bad++; }
        if (right > worst) worst = right;
        if (it->kind == CSM_END) break;
        title = it->title; title_x = x;
        x += 8 * (long)strlen(tr(it->title)) + 16;
        itemw = 0; subw = 0;
        continue;
      }
      if (it->kind == CSM_ITEM || it->kind == CSM_SUB || it->kind == CSM_SUBITEM) {
        const CsCommand *c = it->kind == CSM_SUB ? NULL : cs_cmd_find(it->id);
        const char *label = it->kind == CSM_SUB ? tr(it->title) : c && c->label ? tr(c->label) : "";
        long w = 8 * (long)strlen(label) + 16 + (c && c->key ? 40 : 0) + (c && (c->flags & CSC_TOGGLE) ? 16 : 0) + (it->kind == CSM_SUB ? 16 : 0);
        if (it->kind == CSM_SUBITEM) { if (w > subw) subw = w; }
        else if (w > itemw) itemw = w;
      }
    }
    if (x > 640) { printf("%s the menu titles take %ld pixels of a 640-pixel bar\n", codes[lang], x); bad++; }
    printf("%s menus: titles %ld pixels, the widest menu reaches %ld\n", codes[lang], x, worst);
  }
  return bad;
}
#endif
int main(int argc, char **argv)
{
  int bad = 0;
  FIT_INIT();
  cs_lang_init(words, (int)(sizeof words / sizeof words[0]) - 1);
#ifdef FIT_RIBBON
  if (argc > 1 && !strcmp(argv[1], "ribbon")) bad = ribbon(argc > 2 ? atoi(argv[2]) : 618);
#endif
#ifdef FIT_MENUS
  if (argc > 1 && !strcmp(argv[1], "menus")) bad = menus();
#endif
  return bad != 0;
}
