/*
 * text.h - Jotter's document: plain text as an array of lines (Latin-1), a caret and a selection,
 * editing with undo and redo, finding and replacing, counting, the outline (lines that start with
 * '#'), reading and writing files. Portable C99 with no AmigaOS calls: the front end (amiga/) only
 * shows it and asks it to change; tests/coretest.c tests all of it natively.
 */
#ifndef JOTTER_TEXT_H
#define JOTTER_TEXT_H

#include <stddef.h>

typedef struct { long line, col; } TPos;            /* col: a byte offset in the line */
typedef struct { char *s; long len, cap; } TLine;

/* one change, as undo keeps it: text inserted at a place (or removed from it) */
typedef struct { int insert, chain, typing; TPos at; char *text; long len; TPos caret_before; } TUndo;   /* chain: undone with the one before */

typedef struct {
  TLine *lines; long nlines, caplines;
  TPos caret, anchor;                                 /* the selection runs between anchor and caret */
  TUndo *undo; int nundo, capundo, ndone;             /* undo[0..ndone-1] done, the rest redoable */
  int modified;                                       /* changed since loaded or saved */
  int typing;                                         /* the last step was typing that the next may join */
  int crlf;                                           /* the file had CR LF line ends (kept when saved) */
  char path[256];
} TextDoc;

int  text_init(TextDoc *t);                              /* one empty line; 0 without memory */
void text_free(TextDoc *t);
/* the whole text as one string (lines joined with "\n"), malloc'd; NULL without memory */
char *text_get(const TextDoc *t, long *len);
int  text_set(TextDoc *t, const char *s, long len);      /* replaced (undo cleared) */

/* positions */
TPos text_clamp(const TextDoc *t, TPos p);
int  text_cmp(TPos a, TPos b);
int  text_has_sel(const TextDoc *t);
void text_sel_range(const TextDoc *t, TPos *from, TPos *to);
char *text_sel_get(const TextDoc *t, long *len);         /* the selection, malloc'd ("" if none) */
long text_offset(const TextDoc *t, TPos p);              /* bytes from the start ("\n" counts one) */
TPos text_pos_at(const TextDoc *t, long offset);

/* editing: each is one undo step (typing joins letters into one step until the caret moves) */
int  text_insert(TextDoc *t, const char *s, long len);   /* replaces the selection, caret after */
int  text_delete_sel(TextDoc *t);
int  text_backspace(TextDoc *t);                         /* the selection, or the character before */
int  text_delete(TextDoc *t);                            /* the selection, or the character after */
int  text_undo(TextDoc *t);
int  text_redo(TextDoc *t);
int  text_can_undo(const TextDoc *t);
int  text_can_redo(const TextDoc *t);
void text_break_join(TextDoc *t);                        /* the next typing starts a new undo step */

/* caret movement; extend: keep the anchor (Shift held) */
enum { MV_LEFT, MV_RIGHT, MV_UP, MV_DOWN, MV_HOME, MV_END, MV_TOP, MV_BOTTOM, MV_WORDLEFT, MV_WORDRIGHT };
void text_move(TextDoc *t, int how, int extend);
void text_set_caret(TextDoc *t, TPos p, int extend);
void text_select_all(TextDoc *t);
void text_select_word(TextDoc *t, TPos p);

/* find: from the caret (or the selection's end), forward; wraps round. flags below. The match is
   selected; 0 if none */
#define FIND_CASE 1                                   /* match case */
#define FIND_WORD 2                                   /* whole words only */
#define FIND_BACK 4                                   /* backwards */
int  text_find(TextDoc *t, const char *what, int flags);
/* the selection replaced if it is a match, then the next found; returns 1 if one was replaced */
int  text_replace(TextDoc *t, const char *what, const char *with, int flags);
long text_replace_all(TextDoc *t, const char *what, const char *with, int flags);   /* how many; one undo step */

/* commands on the selection (or the line the caret is on) */
int  text_case(TextDoc *t, int upper);                   /* UPPER or lower case */
int  text_sort_lines(TextDoc *t);                        /* the selected lines (or all) sorted, A first */
int  text_indent(TextDoc *t, int out);                   /* two spaces added at (or taken from) each line's start */

/* counts */
typedef struct { long words, chars, chars_nospace, lines, paragraphs; } TCount;
void text_count(const TextDoc *t, TCount *c);

/* the outline: lines that start with '#', their level (number of #) and line number */
typedef struct { long line; int level; } TOutline;
int  text_outline(const TextDoc *t, TOutline *out, int max);

/* files: Latin-1 bytes; UTF-8 is converted when the file is valid UTF-8 and has characters past
   ASCII (those outside Latin-1 become '?'); CR LF kept as it was. 0 on failure */
int  text_load(TextDoc *t, const char *path);
int  text_save(TextDoc *t, const char *path);
/* text in UTF-8 converted to Latin-1 in place ('?' for what Latin-1 hasn't); its new length */
long utf8_to_latin1(char *s, long len);

#endif
