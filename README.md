# sexp_formatter

<versionBadge>![Version 0.1.1](https://img.shields.io/badge/version-0.1.1-blue.svg)</versionBadge>
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![CI/CD Status Badge](https://github.com/mofosyne/sexp_formatter/actions/workflows/ci.yml/badge.svg)](https://github.com/mofosyne/sexp_prettify/actions)

Prettifies KiCad-like S-expressions according to a KiCADv8-style formatting Via Python.

This is primarily targeted at C but there is multiple implementations in
C/CPP/Python as well that you can use for integration with your code.

```bash
# Build sexp_prettify 
make

# Install sexp_prettify as sexp_prettify system wide
make install

# Remove sexp_prettify cli system wide
make uninstall

# Run all CI/CD build and test (developer)
make cicd
```

Expected Help Message when running `sexp_prettify`

```
S-Expression Formatter (Brian Khuu 2024)

Usage:
  ./sexp_prettify [OPTION]... SOURCE [DESTINATION]
  SOURCE             Source file path. If '-' then use standard stream input
  DESTINATION        Destination file path. If omitted or '-' then use standard stream output

Options:
  -h                 Show Help Message
  -w WRAP_THRESHOLD  Set Wrap Threshold. Must be postive value. (default 72)
  -l COMPACT_LIST    Add To Compact List. Must be a string. 
  -k COLUMN_LIMIT    Add To Compact List Column Limit. Must be positive value. (default 99)
  -s SHORTFORM       Add To Shortform List. Must be a string.
  -p PROFILE         Predefined Style. (kicad, kicad-compact)

Example:
  - Use standard input and standard output. Also use KiCAD's standard compact list and shortform setting.
    ./sexp_prettify -l pts -s font -s stroke -s fill -s offset -s rotate -s scale - -
```

When integrating into your project, copy over `sexp_prettify.c` and `sexp_prettify.h` and use these functions:

```c
// Initialization
bool sexp_prettify_init(struct PrettifySExprState *state, char indent_char, int indent_size, int consecutive_token_wrap_threshold);
// Set Settings
bool sexp_prettify_compact_list_set(struct PrettifySExprState *state, const char **prefixes, int prefixes_entries_count, int column_limit);
bool sexp_prettify_shortform_set(struct PrettifySExprState *state, const char **prefixes, int prefixes_entries_count);
// Process content and output content via PrettifySExprPutcFunc
typedef void (*PrettifySExprPutcFunc)(char c, void *context);
void sexp_prettify(struct PrettifySExprState *state, const char c, PrettifySExprPutcFunc output_func, void *output_func_context);
```

An example of how to use this C library in your project:

First make a callback function

```c
  void putc_handler(char c, void *context_putc) { fputc(c, (FILE *)context_putc); }
```

Then implement this in your main program as a loop of this form

```c
  struct PrettifySExprState state = {0};
  sexp_prettify_init(&state, PRETTIFY_SEXPR_KICAD_DEFAULT_INDENT_CHAR, PRETTIFY_SEXPR_KICAD_DEFAULT_INDENT_SIZE, PRETTIFY_SEXPR_KICAD_DEFAULT_CONSECUTIVE_TOKEN_WRAP_THRESHOLD);

  int ch;
  FILE *dst_file = stdout;
  while ((ch = getc(message)) != EOF)
  {
    sexp_prettify(&state, ch, &putc_handler, dst_file);
  }
```

## Developer

* Run `make` to build all the c and cpp binaries shown above.
* Run `make check` to test all the executable (except for sexp_prettify_kicad_original_cli, I am trying to replace)
* Run `make time` to generate a timing test report

### About Files

* sexp_prettify.py             : This is a python implementation 
* sexp_prettify_kicad_original_cli : This is a cpp original logic from the KiCAD repository as of 2024-12-03
* sexp_prettify_kicad_cli          : This is the new cpp logic from the KiCAD repository being proposed for KiCAD
* sexp_prettify_cpp_cli            : This is a cpp cli wrapper around the c function `sexp_prettify()` in `sexp_prettify.c/h`
* sexp_prettify                : This is a c cli wrapper around the c function `sexp_prettify()` in `sexp_prettify.c/h`

## History

This repo was originally created to run a test to open and save a file using kiutils to investigate a [file corruption issue](https://github.com/mvnmgrx/kiutils/issues/120) and part of the effort is to create a formatter as the output s-expression differs greatly from the input source... so it was hard to determine exactly what was failing.

The formatter ended up being quite a project, but is now complete enough and may become useful for kiutils and other 3rd party kicad python based tools to have an output that relatively matches the output of kicadv8.

This will aim to match as closely as possible to kicad output (which was v8.0.5 when this was written). Just note that this will still differ as there is some inconsistencies in how kicad is outputting their file. The development team for KiCAD is aware of this issue and is tracking it under <https://gitlab.com/kicad/code/kicad/-/issues/15232> however according to craftyjon there is no current big effort to change how the formatting works as of 2024-09-23. So in the meantime, you are recommended to run both files though this sexp_formatter.py if you want to diff both files.

On 2024-12-03 I found out about the kicad official test cases so refactored the codebase to match against it (with some slight adjustment to tokens after list).
