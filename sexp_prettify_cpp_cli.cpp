// KiCADv8 Style Prettify S-Expression Formatter (sexp formatter)
// By Brian Khuu, 2024
// This script reformats KiCad-like S-expressions to match a specific formatting style.
// Note: This script modifies formatting only; it does not perform linting or validation.

#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

extern "C"
{
#include "sexp_prettify.h"
}

typedef enum styleProfile
{
    STYLE_PROFILE_NONE = 0,
    STYLE_PROFILE_KICAD_STANDARD,
    STYLE_PROFILE_KICAD_COMPACT,
} styleProfile;

const char *compact_list_prefixes_kicad[] = {"pts"};
const int compact_list_prefixes_kicad_size = sizeof(compact_list_prefixes_kicad) / sizeof(compact_list_prefixes_kicad[0]);

const char *shortform_prefixes_kicad[] = {"font", "stroke", "fill", "offset", "rotate", "scale"};
const int shortform_prefixes_kicad_size = sizeof(shortform_prefixes_kicad) / sizeof(shortform_prefixes_kicad[0]);

// Display usage instructions
void usage(const std::string &prog_name, bool full = false)
{
    if (full)
    {
        std::cout << "S-Expression Formatter (Brian Khuu 2024)\n\n";
    }

    std::cout << "Usage:\n"
              << "  " << prog_name << " [OPTION]... SOURCE [DESTINATION]\n";
    if (!full)
    {
        std::cout << "  " << prog_name << " -h          Show Full Help Message\n";
    }
    std::cout << "  SOURCE                Source file path. If '-' then use standard stream input\n"
              << "  DESTINATION           Destination file path. If omitted or '-' then use standard stream output\n\n";

    if (full)
    {
        std::cout << "Options:\n"
                  << "  -h                 Show Help Message\n"
                  << "  -w WRAP_THRESHOLD  Set Wrap Threshold. Must be positive value. (default " << PRETTIFY_SEXPR_KICAD_DEFAULT_CONSECUTIVE_TOKEN_WRAP_THRESHOLD << ")\n"
                  << "  -l COMPACT_LIST    Add To Compact List. Must be a string.\n"
                  << "  -k COLUMN_LIMIT    Set Compact List Column Limit. Must be positive value. (default " << PRETTIFY_SEXPR_KICAD_DEFAULT_COMPACT_LIST_COLUMN_LIMIT << ")\n"
                  << "  -s SHORTFORM       Add To Shortform List. Must be a string.\n"
                  << "  -p PROFILE         Predefined Style. (kicad, kicad-compact)\n"
                  << "Example:\n"
                  << "  - Use standard input and standard output. Also use KiCAD's standard compact list and shortform setting.\n"
                  << "    " << prog_name << " -l pts -s font -s stroke -s fill -s offset -s rotate -s scale - -\n";
    }
}

int main(int argc, char **argv)
{
    const std::string prog_name = argv[0];

    int wrap_threshold = PRETTIFY_SEXPR_KICAD_DEFAULT_CONSECUTIVE_TOKEN_WRAP_THRESHOLD;

    std::vector<std::string> compact_list_prefixes;
    int compact_list_prefixes_wrap_threshold = PRETTIFY_SEXPR_KICAD_DEFAULT_COMPACT_LIST_COLUMN_LIMIT;

    std::vector<std::string> shortform_prefixes;

    styleProfile kicad_profile_active = STYLE_PROFILE_NONE;

    // Parse options
    while (optind < argc)
    {
        const int c = getopt(argc, argv, "hl:s:w:k:p:");
        if (c == -1)
        {
            break;
        }

        switch (c)
        {
            case 'h':
            {
                usage(prog_name, true);
                return EXIT_SUCCESS;
            }
            case 'l':
            {
                compact_list_prefixes.emplace_back(optarg);
                break;
            }
            case 's':
            {
                shortform_prefixes.emplace_back(optarg);
                break;
            }
            case 'w':
            {
                wrap_threshold = std::stoi(optarg);
                if (wrap_threshold <= 0)
                {
                    usage(prog_name);
                    return EXIT_FAILURE;
                }
                break;
            }
            case 'k':
            {
                compact_list_prefixes_wrap_threshold = std::stoi(optarg);
                if (compact_list_prefixes_wrap_threshold <= 0)
                {
                    usage(prog_name);
                    return EXIT_FAILURE;
                }
                break;
            }
            case 'p':
            {
                if (strcmp("kicad", optarg) == 0)
                {
                    kicad_profile_active = STYLE_PROFILE_KICAD_STANDARD;
                }
                else if (strcmp("kicad-compact", optarg) == 0)
                {
                    kicad_profile_active = STYLE_PROFILE_KICAD_COMPACT;
                }

                if (kicad_profile_active == STYLE_PROFILE_NONE)
                {
                    fprintf(stderr, "Must be either 'kicad' or 'kicad-compact'");
                    usage(prog_name, false);
                    return EXIT_FAILURE;
                }

                compact_list_prefixes.clear();
                shortform_prefixes.clear();

                if (kicad_profile_active == STYLE_PROFILE_KICAD_STANDARD || kicad_profile_active == STYLE_PROFILE_KICAD_COMPACT)
                {
                    for (int i = 0; i < compact_list_prefixes_kicad_size; i++)
                    {
                        compact_list_prefixes.emplace_back(compact_list_prefixes_kicad[i]);
                    }
                }

                if (kicad_profile_active == STYLE_PROFILE_KICAD_COMPACT)
                {
                    for (int i = 0; i < shortform_prefixes_kicad_size; i++)
                    {
                        shortform_prefixes.emplace_back(shortform_prefixes_kicad[i]);
                    }
                }

                break;
            }
            case '?':
            {
                usage(prog_name);
                return EXIT_FAILURE;
            }
            default:
            {
                usage(prog_name);
                return EXIT_FAILURE;
            }
        }
    }

    // Get fixed arguments
    const char *src_path = nullptr;
    const char *dst_path = nullptr;

    if (optind < argc)
    {
        src_path = argv[optind++];
    }

    if (optind < argc)
    {
        dst_path = argv[optind++];
    }

    if (!src_path)
    {
        usage(prog_name, true);
        return EXIT_SUCCESS;
    }

    // Set up file streams
    std::unique_ptr<std::ifstream> src_file;
    std::istream *src_stream = &std::cin;
    if (src_path && strcmp(src_path, "-") != 0)
    {
        src_file = std::make_unique<std::ifstream>(src_path);
        if (!src_file->is_open())
        {
            std::cerr << "Error opening source file: " << src_path << "\n";
            return EXIT_FAILURE;
        }
        src_stream = src_file.get();
    }

    std::unique_ptr<std::ofstream> dst_file;
    std::ostream *dst_stream = &std::cout;
    if (dst_path && strcmp(dst_path, "-") != 0)
    {
        dst_file = std::make_unique<std::ofstream>(dst_path);
        if (!dst_file->is_open())
        {
            std::cerr << "Error opening destination file: " << dst_path << "\n";
            return EXIT_FAILURE;
        }
        dst_stream = dst_file.get();
    }

    // Initialize state
    PrettifySExprState state = {0};
    std::vector<const char *> compact_list_ptrs;
    std::vector<const char *> shortform_ptrs;

    assert(sexp_prettify_init(&state, PRETTIFY_SEXPR_KICAD_DEFAULT_INDENT_CHAR, PRETTIFY_SEXPR_KICAD_DEFAULT_INDENT_SIZE, wrap_threshold));

    if (!compact_list_prefixes.empty())
    {
        for (const auto &prefix : compact_list_prefixes)
        {
            compact_list_ptrs.push_back(prefix.c_str());
        }

        assert(sexp_prettify_compact_list_set(&state, compact_list_ptrs.data(), compact_list_ptrs.size(), compact_list_prefixes_wrap_threshold));
    }

    if (!shortform_prefixes.empty())
    {
        for (const auto &prefix : shortform_prefixes)
        {
            shortform_ptrs.push_back(prefix.c_str());
        }

        assert(sexp_prettify_shortform_set(&state, shortform_ptrs.data(), shortform_ptrs.size()));
    }

    // Define the lambda closer to usage
    auto putc_handler = [](char ch, void *context_putc)
    {
        auto &out_stream = *static_cast<std::ostream *>(context_putc);
        out_stream.put(ch);
    };

    // Process the input
    char c;
    while (src_stream->get(c))
    {
        sexp_prettify(&state, c, putc_handler, dst_stream);
    }

    return EXIT_SUCCESS;
}
