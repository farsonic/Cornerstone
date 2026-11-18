# 14. Following an upstream program: the gap document

Each program follows an upstream "Craft" program (Quill: WordCraft; Ledger: GridCraft; Easel:
PhotoCraft; Folio: PDFCraft), which itself follows a PC original. The **gap document** is the plan
of record: every command and feature the upstream program has, each marked with how far ours goes,
with evidence. Work is chosen from it, and it is recounted after every merge.

## Making one

1. **Take the upstream program's own catalogue** where it has one (WordCraft's feature catalogue,
   checked by its own parity tool; GridCraft's function list; PDFCraft's tool catalogue), then its
   ribbon or menus as it actually builds them, its options, keys and languages, and its parity
   documents. Note the commit it was taken from and the date.
2. **One line per command or feature**, grouped by where it lives (a section per tab, per menu, per
   panel):

   ```
   ### Home (115: ✅ 113 · 🟨 1 · ⬜ 0 · ➖ 1)
   - ✅ Bold `format.bold`: ID_BOLD (amiga/ids.h), core/fmt.c fmt_toggle
   - 🟨 Change Case `format.changeCase`: five of WordCraft's six modes; Title Case missing
   - ⬜ Emphasis Mark `format.emphasisMark` [WP27]
   - ➖ Dictate: needs a network service
   ```

3. **The marks**: ✅ done, 🟨 partly (say what is missing), ⬜ missing (with the work package that
   will close it in brackets), ➖ left out on purpose (printing, scripting, network and cloud
   services; or the upstream program doesn't have it either).
4. **Evidence for every ✅**: the command id or the function that does it, so anyone can check.
   A ⬜ was searched for in the source and not found.
5. **A summary** at the top: the totals, a table per section, the strongest and weakest areas,
   the biggest risks for documents people exchange.

## Keeping it

- **Recount after every merge**: `python3 kit/shared/tools/gapcount.py docs/<upstream>-gap.md
  --write` rewrites each section heading's counts, the summary table's rows, its "All" row and the
  "- **Coverage:**" line.
- **Sync with upstream** regularly: take its log since the commit last compared, add the new
  commands (marked `(new 2026-11)`) and the features its new work brings, and record the commit.
  Quill's 2026-11-18 sync took twelve new commands from 110 merged pull requests.
- **Work packages**: group the missing lines into packages (WP1 fields, WP2 references...), each
  a branch with its own command block, its own `wp_*.c` files and tests, so packages can be built
  in parallel and merged in any order (Quill's `amiga/wphooks.h` explains how one plugs in).
- **Integration notes**: where packages touch the same thing (Quill's `settings.xml` element
  order), the gap document or NEXT.md says who owns it.

## Where it stands in the family

| Program | Upstream | Gap document | Coverage |
|---|---|---|---|
| Quill | WordCraft `22a73d9` | `docs/wordcraft-gap.md` | 512 of 543 ribbon commands done, 3 partly, 28 left out |
| Ledger | GridCraft `4c388df` | `docs/gridcraft-gap.md`, `FEATURES.md` | every function and ribbon command but the scripting tools |
| Easel | PhotoCraft | `docs/photocraft-gap.md` | area by area; its summary wants recounting |
| Folio | PDFCraft `2251a75` | `docs/pdfcraft-gap.md` | 121 of 130 applicable tools done, 9 partly |
