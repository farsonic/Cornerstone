# 15. The CLAUDE.md template

Every program has a `CLAUDE.md` at its root: the house rules and where work stands, for anyone,
person or agent, picking the work up. The family's are all in one shape (Quill's, Ledger's,
Easel's, Folio's), and a new program starts from `kit/shared/templates/CLAUDE.md`: copy it,
replace the capitals (PROGRAM, UPSTREAM, WHAT-IT-IS...), and keep it current.

Its sections, and what each is for:

| Section | What goes in it |
|---|---|
| Opening paragraph | What the program is, which platform, which upstream program it follows, the licence, what to read next. |
| Layout | A table of the tree: what each directory holds, the important files by name. |
| Build and test | The exact command CI runs and what it should print; the platform build and the toolchain's paths and where to get each part (and what may not be redistributed: the MUI SDK); the release command; how to test on the platform. |
| Rules of the project | The compiler's pitfalls, the command-table rule, the screen size, translation, icons, what a feature needs (test, gap line, README), what is left out on purpose, no `rm -rf`, the commit identity, no personal email. |
| Where work stands | What is released, merged, on branches; what was and wasn't checked on real hardware; what comes next. The part that changes most: update it at the end of every piece of work. |
| Issues | How bugs come in and are worked: reproduce natively, a failing test, a branch named `issue-N-...`, a pull request saying `Fixes #N`; private attachments never committed. |

A long project may keep a `NEXT.md` beside it for the detail of where work stopped (Quill's lists
its branches, integration fixes and pitfalls); CLAUDE.md then points to it.

The template, in full: [`kit/shared/templates/CLAUDE.md`](../../kit/shared/templates/CLAUDE.md).
The other templates: `kit/shared/templates/workflows/tests.yml` (CI: the native tests),
`workflows/claude.yml` (mentioning `@claude` on GitHub), `ISSUE_TEMPLATE/` (bug report and feature
request forms).
