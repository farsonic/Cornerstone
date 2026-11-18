/*
 * cs_buffer.c - a document view's off-screen buffer, as Folio's and Quill's views have it. The view
 * draws into a bitmap made like the window's (on a graphics card, in its memory) and shows it in
 * one blit, so a redraw never flickers through the background first. Scrolling moves the buffer's
 * picture with ScrollRaster (the blitter's work, the Amiga's own or the card's) and only the strips
 * that came into view are drawn. The bitmap has a layer because graphics.library clips only
 * through one: drawing past a bare bitmap's edge would write over memory. The layer starts at 0,0,
 * so window coordinates work in it unchanged. Without memory for it, drawing goes to the window.
 */
#include <string.h>
#include <graphics/gfx.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/muimaster.h>
#include "cs_amiga.h"
#include "cs_classes.h"

void cs_buf_free(CsBuffer *b)
{
  if (b->ly) { InstallClipRegion(b->ly, NULL); DeleteLayer(0, b->ly); }
  if (b->li) DisposeLayerInfo(b->li);
  if (b->base) DisposeRegion(b->base);
  if (b->bm) { WaitBlit(); FreeBitMap(b->bm); }
  memset(b, 0, sizeof *b);
}
/* the buffer for the view's box as it is now: made again when the box has changed */
static int ready(CsBuffer *b, Object *obj)
{
  struct BitMap *wbm = _rp(obj)->BitMap;
  struct Rectangle r;
  if (b->ly && b->x0 == _mleft(obj) && b->y0 == _mtop(obj) && b->x1 == _mright(obj) && b->y1 == _mbottom(obj)) return 1;
  cs_buf_free(b);
  if (_mwidth(obj) <= 0 || _mheight(obj) <= 0) return 0;
  if (!(b->bm = AllocBitMap((ULONG)_mright(obj) + 1, (ULONG)_mbottom(obj) + 1, GetBitMapAttr(wbm, BMA_DEPTH), 0, wbm)) ||
      !(b->li = NewLayerInfo()) ||
      !(b->ly = CreateUpfrontLayer(b->li, b->bm, 0, 0, _mright(obj), _mbottom(obj), LAYERSIMPLE, NULL)) ||
      !(b->base = NewRegion())) { cs_buf_free(b); return 0; }
  r.MinX = (WORD)_mleft(obj); r.MinY = (WORD)_mtop(obj); r.MaxX = (WORD)_mright(obj); r.MaxY = (WORD)_mbottom(obj);
  if (!OrRectRegion(b->base, &r)) { cs_buf_free(b); return 0; }
  InstallClipRegion(b->ly, b->base);
  b->x0 = _mleft(obj); b->y0 = _mtop(obj); b->x1 = _mright(obj); b->y1 = _mbottom(obj);
  b->ok = 1;
  return 1;
}
struct RastPort *cs_buf_begin(CsBuffer *b, Object *obj)
{
  if (!ready(b, obj)) return _rp(obj);
  SetFont(b->ly->rp, _rp(obj)->Font);
  return b->ly->rp;
}
void cs_buf_show(CsBuffer *b, Object *obj, LONG x0, LONG y0, LONG x1, LONG y1)
{
  if (!b->ly) return;                       /* (drawn straight into the window) */
  if (x0 < _mleft(obj)) x0 = _mleft(obj);
  if (y0 < _mtop(obj)) y0 = _mtop(obj);
  if (x1 > _mright(obj)) x1 = _mright(obj);
  if (y1 > _mbottom(obj)) y1 = _mbottom(obj);
  if (x1 < x0 || y1 < y0) return;
  ClipBlit(b->ly->rp, x0, y0, _rp(obj), x0, y0, x1 - x0 + 1, y1 - y0 + 1, 0xC0);
}
void cs_buf_clip(CsBuffer *b, const struct Rectangle *boxes, int n)
{
  struct Region *r;
  int k;
  if (!b->ly) return;
  if (!n) { InstallClipRegion(b->ly, b->base); return; }
  if (!(r = NewRegion())) return;
  for (k = 0; k < n; k++) OrRectRegion(r, &boxes[k]);
  AndRegionRegion(b->base, r);
  /* (the region installed is kept by the layer until the next install: the old one freed then) */
  {
    struct Region *old = InstallClipRegion(b->ly, r);
    if (old && old != b->base) DisposeRegion(old);
  }
}
int cs_buf_scroll(CsBuffer *b, Object *obj, LONG dx, LONG dy, struct Rectangle *box, int *nb)
{
  LONG l = _mleft(obj), t = _mtop(obj), r = _mright(obj), bo = _mbottom(obj);
  *nb = 0;
  if (!ready(b, obj)) return 0;
  if (dx >= _mwidth(obj) * 3 / 4 || -dx >= _mwidth(obj) * 3 / 4 || dy >= _mheight(obj) * 3 / 4 || -dy >= _mheight(obj) * 3 / 4) return 0;
  if (!dx && !dy) return 1;
  {
    struct Region *old = InstallClipRegion(b->ly, NULL);      /* the whole layer is the view: nothing to clip */
    if (old && old != b->base) DisposeRegion(old);
  }
  ScrollRaster(b->ly->rp, dx, dy, l, t, r, bo);
  if (dy > 0) { box[*nb].MinX = (WORD)l; box[*nb].MaxX = (WORD)r; box[*nb].MinY = (WORD)(bo - dy + 1); box[*nb].MaxY = (WORD)bo; (*nb)++; }
  if (dy < 0) { box[*nb].MinX = (WORD)l; box[*nb].MaxX = (WORD)r; box[*nb].MinY = (WORD)t; box[*nb].MaxY = (WORD)(t - dy - 1); (*nb)++; }
  if (dx > 0) { box[*nb].MinX = (WORD)(r - dx + 1); box[*nb].MaxX = (WORD)r; box[*nb].MinY = (WORD)t; box[*nb].MaxY = (WORD)bo; (*nb)++; }
  if (dx < 0 && *nb < 2) { box[*nb].MinX = (WORD)l; box[*nb].MaxX = (WORD)(l - dx - 1); box[*nb].MinY = (WORD)t; box[*nb].MaxY = (WORD)bo; (*nb)++; }
  InstallClipRegion(b->ly, b->base);
  return 1;
}
