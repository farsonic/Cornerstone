# 11. Interface icons on the Amiga

The icon language is shared chapter 12. On the Amiga:

```c
void cs_icon(struct RastPort *rp, int icon, LONG x, LONG y, LONG size, int dim);
void cs_icon_draw(struct RastPort *rp, const char *src, LONG x, LONG y, LONG size, int dim);
void cs_icon_pen(struct RastPort *rp, int icon, LONG x, LONG y, LONG size, LONG pen);
```

- `cs_icon` runs the portable interpreter with callbacks that draw through graphics.library:
  `Move`/`Draw` for lines and polylines, the four sides for `R`, `RectFill` for `F` (grown by
  `size/16 - 1` above 16 pixels), `DrawEllipse` for `O`, `RectFill` spans for `E`, and `Text` in
  `cs_bold()` or `cs_font()` with `SetSoftStyle` for `B`, `I` and `U` (bold smeared by the style when
  there is no bold font).
- Each ink becomes the theme's pen (`CS_T_ICON`, `CS_T_ACCENT`...); `dim` makes every ink but white
  `CS_T_DISABLED`; `cs_icon_pen` draws every ink in one pen and white in the accent.
- The usual sizes: **16** for small items, Quick Access, pop-up lists, pane headers; **24** for
  large ribbon items and collapsed groups; **8** for drop-down arrows; **10-12** for ticks and close
  boxes. Large text doesn't change icon sizes (the rows grow round them).
- Icons a program adds are `CS_IC_APP + n`, handed over with `cs_icon_set_app()`; an item of a table
  can also carry its own string (`cs_icon_draw`).
- **Check new icons** with the previewer before the emulator:
  `python3 kit/shared/tools/iconsheet.py /tmp/sheet.png --root .` draws them exactly as
  `cs_icon` scales them, in both themes, dimmed and mirrored.
