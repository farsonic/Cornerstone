/*
 * clip.c - the Amiga clipboard for text: unit 0, an IFF FORM FTXT with one CHRS chunk, through
 * iffparse.library (opened only while it is used). Other programs (editors, the Shell's Amiga-V)
 * read and write the same.
 */
#include <string.h>
#include <stdlib.h>
#include <libraries/iffparse.h>
#include <proto/exec.h>
#include <proto/iffparse.h>
#include "jotter.h"

struct Library *IFFParseBase;
#define ID_FTXT MAKE_ID('F', 'T', 'X', 'T')
#define ID_CHRS MAKE_ID('C', 'H', 'R', 'S')

static struct IFFHandle *open_clip(LONG mode)
{
  struct IFFHandle *h;
  if (!(IFFParseBase = OpenLibrary((STRPTR)"iffparse.library", 39))) return NULL;
  if (!(h = AllocIFF())) { CloseLibrary(IFFParseBase); IFFParseBase = NULL; return NULL; }
  if (!(h->iff_Stream = (ULONG)OpenClipboard(0))) { FreeIFF(h); CloseLibrary(IFFParseBase); IFFParseBase = NULL; return NULL; }
  InitIFFasClip(h);
  if (OpenIFF(h, mode)) { CloseClipboard((struct ClipboardHandle *)h->iff_Stream); FreeIFF(h); CloseLibrary(IFFParseBase); IFFParseBase = NULL; return NULL; }
  return h;
}
static void close_clip(struct IFFHandle *h)
{
  CloseIFF(h);
  CloseClipboard((struct ClipboardHandle *)h->iff_Stream);
  FreeIFF(h);
  CloseLibrary(IFFParseBase);
  IFFParseBase = NULL;
}
int clip_write(const char *s, long len)
{
  struct IFFHandle *h = open_clip(IFFF_WRITE);
  int ok;
  if (!h) return 0;
  ok = !PushChunk(h, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN) && !PushChunk(h, 0, ID_CHRS, IFFSIZE_UNKNOWN) &&
       WriteChunkBytes(h, (APTR)s, len) == len && !PopChunk(h) && !PopChunk(h);
  close_clip(h);
  return ok;
}
char *clip_read(long *len)
{
  struct IFFHandle *h = open_clip(IFFF_READ);
  char *out = NULL;
  long n = 0;
  if (!h) return NULL;
  if (!StopChunk(h, ID_FTXT, ID_CHRS)) {
    while (!ParseIFF(h, IFFPARSE_SCAN)) {            /* every CHRS chunk, joined */
      struct ContextNode *cn = CurrentChunk(h);
      char *more;
      if (!cn || cn->cn_Size <= 0) continue;
      if (!(more = (char *)realloc(out, (size_t)(n + cn->cn_Size + 1)))) break;
      out = more;
      n += ReadChunkBytes(h, out + n, cn->cn_Size);
    }
  }
  close_clip(h);
  if (out) { out[n] = 0; *len = n; }
  return out;
}
