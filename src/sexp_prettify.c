// KiCADv8 Style Prettify S-Expression Formatter (sexp formatter)
// By Brian Khuu, 2024
// This script reformats KiCad-like S-expressions to match a specific formatting style.
// Note: This script modifies formatting only; it does not perform linting or validation.

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include "sexp_prettify.h"

bool sexp_prettify_init(struct PrettifySExprState *state, char indent_char, int indent_size, int consecutive_token_wrap_threshold)
{
    if (indent_char == '\0')
    {
        return false;
    }

    if (indent_size <= 0)
    {
        return false;
    }

    state->indent_char = indent_char;
    state->indent_size = indent_size;
    state->consecutive_token_wrap_threshold = consecutive_token_wrap_threshold;

    return true;
}

bool sexp_prettify_compact_list_set(struct PrettifySExprState *state, const char **prefixes, int prefixes_entries_count, int column_limit)
{
    if (prefixes_entries_count <= 0)
    {
        return false;
    }

    for (int i = 0; i < prefixes_entries_count; i++)
    {
        if (strlen(prefixes[i]) > PRETTIFY_SEXPR_PREFIX_BUFFER_SIZE)
        {
            return false;
        }
    }

    state->compact_list_prefixes = prefixes;
    state->compact_list_prefixes_entries_count = prefixes_entries_count;
    state->compact_list_column_limit = column_limit;

    return true;
}

bool sexp_prettify_shortform_set(struct PrettifySExprState *state, const char **prefixes, int prefixes_entries_count)
{
    if (prefixes_entries_count <= 0)
    {
        return false;
    }

    for (int i = 0; i < prefixes_entries_count; i++)
    {
        if (strlen(prefixes[i]) > PRETTIFY_SEXPR_PREFIX_BUFFER_SIZE)
        {
            return false;
        }
    }

    state->shortform_prefixes = prefixes;
    state->shortform_prefixes_entries_count = prefixes_entries_count;

    return true;
}

/*
 * Formatting rules (Based on KiCAD S-Expression Style Guide):
 * - All extra (non-indentation) whitespace is trimmed.
 * - Indentation is one tab.
 * - Starting a new list (open paren) starts a new line with one deeper indentation.
 * - Lists with no inner lists go on a single line.
 * - End of multi-line lists (close paren) goes on a single line at the same indentation as its start.
 * - If fixed-indent mode is active and within column limits, parentheses will stay on the same line.
 * - Closing parentheses align with the indentation of the corresponding opening parenthesis.
 * - Quoted strings are treated as a single token.
 * - Tokens exceeding the column threshold are moved to the next line.
 * - Singular elements are inlined (e.g., `()`).
 * - Output ends with a newline to ensure POSIX compliance.
 *
 * For example:
 * (first
 *     (second
 *         (third list)
 *         (another list)
 *     )
 *     (fifth)
 *     (sixth thing with lots of tokens
 *         (first sub list)
 *         (second sub list)
 *         and another series of tokens
 *     )
 * )
 */
void sexp_prettify(struct PrettifySExprState *state, const char c, PrettifySExprPutcFunc output_func, void *output_func_context)
{
    // Parse quoted string
    if (state->in_quote || c == '"')
    {
        // Handle quoted strings
        if (state->space_pending)
        {
            // Add space before this quoted string
            output_func(' ', output_func_context);
            state->column += 1;
            state->space_pending = false;
        }

        if (state->escape_next_char)
        {
            // Escaped Char
            state->escape_next_char = false;
        }
        else if (c == '\\')
        {
            // Escape Next Char
            state->escape_next_char = true;
        }
        else if (c == '"')
        {
            // End of quoted string mode
            state->in_quote = !state->in_quote;
        }

        output_func(c, output_func_context);
        state->column += 1;
        state->c_out_prev = c;
        return;
    }

    // Parse space and newlines
    if (isspace(c))
    {
        // Handle spaces and newlines
        state->space_pending = true;

        if (state->scanning_for_prefix)
        {
            state->prefix_buffer[state->prefix_count_in_buffer] = '\0';

            // Check if we got a match against an expected prefix for fixed indent mode
            if (state->compact_list_prefixes)
            {
                for (int key_lookup_index = 0; key_lookup_index < state->compact_list_prefixes_entries_count; key_lookup_index++)
                {
                    if (strcmp(state->prefix_buffer, state->compact_list_prefixes[key_lookup_index]) == 0)
                    {
                        state->compact_list_mode = true;
                        state->compact_list_indent = state->indent;
                        break;
                    }
                }
            }

            // Check if we got a match against an expected prefix for fixed indent mode
            if (state->shortform_prefixes)
            {
                for (int key_lookup_index = 0; key_lookup_index < state->shortform_prefixes_entries_count; key_lookup_index++)
                {
                    if (strcmp(state->prefix_buffer, state->shortform_prefixes[key_lookup_index]) == 0)
                    {
                        state->shortform_mode = true;
                        state->shortform_indent = state->indent;
                        break;
                    }
                }
            }

            state->scanning_for_prefix = false;
        }
        return;
    }

    // Parse Opening parentheses
    if (c == '(')
    {
        // Handle opening parentheses
        state->space_pending = false;

        // Pre Opening parentheses Actions
        if (state->compact_list_mode)
        {
            // In fixed indent, visually compact mode
            if ((state->column < state->compact_list_column_limit && state->c_out_prev == ')') || state->compact_list_column_limit == 0)
            {
                // Is a consecutive list and still within column limit (or column limit disabled)
                output_func(' ', output_func_context);
                state->column += 1;
                state->space_pending = false;
            }
            else
            {
                // List is either beyond column limit or not after another list
                // Move this list to the next line

                output_func('\n', output_func_context);
                state->column = 0;

                for (unsigned int j = 0; j < (state->compact_list_indent * state->indent_size); ++j)
                {
                    output_func(state->indent_char, output_func_context);
                }
                state->column += state->compact_list_indent * state->indent_size;
            }
        }
        else if (state->shortform_mode)
        {
            // In one liner mode
            output_func(' ', output_func_context);
            state->column += 1;
            state->space_pending = false;
        }
        else
        {
            // Start scanning for prefix for special list handling
            state->scanning_for_prefix = true;
            state->prefix_count_in_buffer = 0;

            if (state->indent > 0)
            {
                output_func('\n', output_func_context);
                state->column = 0;

                for (unsigned int j = 0; j < (state->indent * state->indent_size); ++j)
                {
                    output_func(state->indent_char, output_func_context);
                }
                state->column += state->indent * state->indent_size;
            }
        }

        state->singular_element = true;
        state->indent++;

        output_func('(', output_func_context);
        state->column += 1;

        state->c_out_prev = '(';
        return;
    }

    // Parse Closing Brace
    if (c == ')')
    {
        const bool curr_shortform_mode = state->shortform_mode;

        // Handle closing parentheses
        state->space_pending = false;
        state->scanning_for_prefix = false;

        if (state->indent > 0)
        {
            state->indent--;
        }

        // Check if compact list mode has completed
        if (state->compact_list_mode && state->indent < state->compact_list_indent)
        {
            state->compact_list_mode = false;
        }

        // Check if we have finished with a shortform list
        if (state->shortform_mode && state->indent < state->shortform_indent)
        {
            state->shortform_mode = false;
        }

        if (state->wrapped_list)
        {
            // This was a list with wrapped tokens so is already indented
            output_func('\n', output_func_context);
            state->column = 0;

            for (unsigned int j = 0; j < (state->indent * state->indent_size); ++j)
            {
                output_func(state->indent_char, output_func_context);
            }
            state->column += state->indent * state->indent_size;

            if (state->singular_element)
            {
                // End of singular element
                state->singular_element = false;
            }

            state->wrapped_list = false;
        }
        else
        {
            // Normal List
            if (state->singular_element)
            {
                // End of singular element
                state->singular_element = false;
            }
            else if (!curr_shortform_mode)
            {
                // End of a parent element
                output_func('\n', output_func_context);
                state->column = 0;

                for (unsigned int j = 0; j < (state->indent * state->indent_size); ++j)
                {
                    output_func(state->indent_char, output_func_context);
                }
                state->column += state->indent * state->indent_size;
            }
        }

        output_func(')', output_func_context);
        state->column += 1;

        if (state->indent <= 0)
        {
            // Cap Root Element
            output_func('\n', output_func_context);
            state->column = 0;
        }

        state->c_out_prev = ')';
        return;
    }

    // Parse Characters
    if (c != '\0')
    {
        // Handle other characters

        // Pre character actions
        if (state->c_out_prev == ')' && !state->shortform_mode)
        {
            // Is Bare token after a list that should be on next line
            // Dev Note: In KiCAD this may indicate a flag bug
            output_func('\n', output_func_context);
            state->column = 0;

            for (unsigned int j = 0; j < (state->indent * state->indent_size); ++j)
            {
                output_func(state->indent_char, output_func_context);
            }
            state->column += state->indent * state->indent_size;

            state->space_pending = false;
        }
        else if (state->space_pending && !state->shortform_mode && !state->compact_list_mode && state->consecutive_token_wrap_threshold != 0 &&
                 state->column >= state->consecutive_token_wrap_threshold)
        {
            // Token is above wrap threshold. Move token to next line (If token wrap threshold is zero then this feature is disabled)
            state->wrapped_list = true;

            output_func('\n', output_func_context);
            state->column = 0;

            for (unsigned int j = 0; j < (state->indent * state->indent_size); ++j)
            {
                output_func(state->indent_char, output_func_context);
            }
            state->column += state->indent * state->indent_size;

            state->space_pending = false;
        }
        else if (state->space_pending && state->c_out_prev != '(')
        {
            // Space was pending
            output_func(' ', output_func_context);
            state->column += 1;

            state->space_pending = false;
        }

        // Add to prefix scanning buffer if scanning for special list handling detection
        if (state->scanning_for_prefix)
        {
            if (state->prefix_count_in_buffer < (PRETTIFY_SEXPR_PREFIX_BUFFER_SIZE - 1))
            {
                state->prefix_buffer[state->prefix_count_in_buffer] = c;
                state->prefix_count_in_buffer++;
            }
        }

        // Add character to list
        output_func(c, output_func_context);
        state->column += 1;

        state->c_out_prev = c;
        return;
    }
}
