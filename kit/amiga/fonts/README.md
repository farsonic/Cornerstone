# The family's screen fonts

QuillSans (regular and bold) and QuillMono are Amiga bitmap fonts rendered from Liberation Sans and
Liberation Mono by Quill's `tools/mkamigafonts.py`. A rendering is a modified version under the SIL
Open Font License 1.1, so it may not carry the original's name: hence "Quill...". The licence and
the original copyright notices are in `LICENSE-fonts.txt`.

Only the sizes Cornerstone's theme and example use are here (QuillSans 9, 11, 12, 15, 27 and 35;
QuillSansB 11 and 15; QuillMono 11, 14 and 17). A program that needs more copies them from Quill's
`fonts-amiga/` or makes them with `mkamigafonts.py`, and writes the `.font` files listing the sizes
it ships (each `.font` names only the sizes in its drawer).
