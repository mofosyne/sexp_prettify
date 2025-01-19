// KiCADv8 Style Prettify S-Expression Formatter (sexp formatter)
// By Brian Khuu, 2024
// This script reformats KiCad-like S-expressions to match a specific formatting style.
// Note: This script modifies formatting only; it does not perform linting or validation.

#ifndef SEXP_PRETTIFY
#define SEXP_PRETTIFY
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

// Default Suggested Values (Based On KiCADv8)
#define PRETTIFY_SEXPR_KICAD_DEFAULT_CONSECUTIVE_TOKEN_WRAP_THRESHOLD 72
#define PRETTIFY_SEXPR_KICAD_DEFAULT_COMPACT_LIST_COLUMN_LIMIT 99
#define PRETTIFY_SEXPR_KICAD_DEFAULT_INDENT_CHAR '\t'
#define PRETTIFY_SEXPR_KICAD_DEFAULT_INDENT_SIZE 1

// The size of this should be larger than the largest prefixes in compact_list_prefixes
#define PRETTIFY_SEXPR_PREFIX_BUFFER_SIZE 256

// Prettify S-Expr State
struct PrettifySExprState
{
    // Settings
    int consecutive_token_wrap_threshold; ///< Tokens exceeding this wrap threshold will be shifted to the next line. If 0 then this wrapping feature is disabled

    // Settings: Compact Lists (This deals with lists with lots and lots of sublists and making it visually more compact)
    const char **compact_list_prefixes;
    int compact_list_prefixes_entries_count;
    int compact_list_column_limit; ///< Lists exceeding this wrap threshold will be shifted to the next line. If 0 then this wrapping feature for compact list is disabled

    // Settings: Shortforms (This deals with lists that is small enough and should be)
    const char **shortform_prefixes;
    int shortform_prefixes_entries_count;

    // Settings: Indent
    char indent_char;
    int indent_size;

    // Parsing Position Tracking
    unsigned int indent;
    unsigned int column;
    char c_out_prev;

    // Parsing state
    bool in_quote;
    bool escape_next_char;
    bool singular_element;
    bool space_pending;
    bool wrapped_list;

    // Prefix scanner to check if a list should be specially handled
    bool scanning_for_prefix;
    int prefix_count_in_buffer;
    char prefix_buffer[PRETTIFY_SEXPR_PREFIX_BUFFER_SIZE];

    // Fixed indent feature to place multiple elements in the same line for compactness
    bool compact_list_mode;
    unsigned int compact_list_indent;

    // Fixed indent feature to place multiple elements in the same line for compactness
    bool shortform_mode;
    unsigned int shortform_indent;
};

typedef void (*PrettifySExprPutcFunc)(char c, void *context);

bool sexp_prettify_init(struct PrettifySExprState *state, char indent_char, int indent_size, int consecutive_token_wrap_threshold);
bool sexp_prettify_compact_list_set(struct PrettifySExprState *state, const char **prefixes, int prefixes_entries_count, int column_limit);
bool sexp_prettify_shortform_set(struct PrettifySExprState *state, const char **prefixes, int prefixes_entries_count);
void sexp_prettify(struct PrettifySExprState *state, const char c, PrettifySExprPutcFunc output_func, void *output_func_context);

#ifdef __cplusplus
}
#endif
#endif
