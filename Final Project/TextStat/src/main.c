/*
 * main.c - command-line interface for textstat.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "textstat.h"

#define PROG_NAME "textstat"

enum {
    EXIT_RUNTIME = 1, /* a file could not be read, out of memory, ... */
    EXIT_USAGE   = 2  /* bad command-line arguments                   */
};

typedef struct {
    size_t top;
    size_t min_len;
    int    json;
} Options;

static void usage(FILE *out)
{
    fputs(
        "Usage: " PROG_NAME " [OPTIONS] [FILE...]\n"
        "\n"
        "Count bytes, lines and words, and list the most frequent words.\n"
        "Reads standard input when no FILE is given or when FILE is '-'.\n"
        "\n"
        "Options:\n"
        "  -t, --top N          show the N most frequent words (default 10, 0 = none)\n"
        "  -m, --min-length N   ignore words shorter than N in the frequency list\n"
        "                       (default 1)\n"
        "  -j, --json           print results as JSON\n"
        "  -h, --help           show this help and exit\n"
        "  -V, --version        show version and exit\n"
        "\n"
        "Exit status: 0 success, 1 file or runtime error, 2 usage error.\n",
        out);
}

/* Parse a non-negative decimal integer. Returns 0 on success, -1 otherwise. */
static int parse_size(const char *s, size_t *out)
{
    char *end;
    unsigned long v;

    if (s[0] < '0' || s[0] > '9') /* rejects "", "-1", "+1", " 1" */
        return -1;

    errno = 0;
    v = strtoul(s, &end, 10);
    if (errno != 0 || *end != '\0')
        return -1;

    *out = (size_t)v;
    return 0;
}

/* Print `s` as a JSON string literal. */
static void print_json_string(FILE *out, const char *s)
{
    fputc('"', out);
    for (; *s != '\0'; s++) {
        unsigned char c = (unsigned char)*s;
        switch (c) {
        case '"':  fputs("\\\"", out); break;
        case '\\': fputs("\\\\", out); break;
        case '\n': fputs("\\n", out);  break;
        case '\r': fputs("\\r", out);  break;
        case '\t': fputs("\\t", out);  break;
        default:
            if (c < 0x20)
                fprintf(out, "\\u%04x", c);
            else
                fputc(c, out);
        }
    }
    fputc('"', out);
}

static void report_text(const char *label, const TextSummary *s,
                        const WordCount *top, size_t top_len)
{
    printf("== %s ==\n", label);
    printf("Bytes:         %zu\n", s->bytes);
    printf("Lines:         %zu\n", s->lines);
    printf("Words:         %zu\n", s->words);
    printf("Unique words:  %zu\n", s->unique_words);
    printf("Longest line:  %zu\n", s->longest_line);

    if (top_len > 0) {
        printf("\nTop %zu word%s:\n", top_len, top_len == 1 ? "" : "s");
        for (size_t i = 0; i < top_len; i++)
            printf("  %2zu. %-20s %zu\n", i + 1, top[i].word, top[i].count);
    }
    putchar('\n');
}

static void report_json(const char *label, const TextSummary *s,
                        const WordCount *top, size_t top_len)
{
    fputs("  {\n    \"file\": ", stdout);
    print_json_string(stdout, label);
    printf(",\n    \"bytes\": %zu,\n", s->bytes);
    printf("    \"lines\": %zu,\n", s->lines);
    printf("    \"words\": %zu,\n", s->words);
    printf("    \"unique_words\": %zu,\n", s->unique_words);
    printf("    \"longest_line\": %zu,\n", s->longest_line);
    fputs("    \"top_words\": [", stdout);
    for (size_t i = 0; i < top_len; i++) {
        fputs(i == 0 ? "\n      " : ",\n      ", stdout);
        fputs("{\"word\": ", stdout);
        print_json_string(stdout, top[i].word);
        printf(", \"count\": %zu}", top[i].count);
    }
    fputs(top_len > 0 ? "\n    ]\n  }" : "]\n  }", stdout);
}

/* Analyze one stream and print its report. Returns 0 on success. */
static int process(const char *label, FILE *fp, const Options *opt, int *json_first)
{
    TextSummary summary;
    WordCount *top = NULL;
    size_t top_len = 0;
    int rc = -1;

    TextStats *ts = textstat_create(opt->min_len);
    if (ts == NULL) {
        fprintf(stderr, PROG_NAME ": %s: %s\n", label, strerror(ENOMEM));
        return -1;
    }

    if (textstat_read(ts, fp) != 0) {
        fprintf(stderr, PROG_NAME ": %s: read error: %s\n", label,
                errno ? strerror(errno) : "unknown error");
        goto done;
    }
    if (textstat_top(ts, opt->top, &top, &top_len) != 0) {
        fprintf(stderr, PROG_NAME ": %s: %s\n", label, strerror(ENOMEM));
        goto done;
    }

    textstat_summary(ts, &summary);

    if (opt->json) {
        fputs(*json_first ? "\n" : ",\n", stdout);
        *json_first = 0;
        report_json(label, &summary, top, top_len);
    } else {
        report_text(label, &summary, top, top_len);
    }
    rc = 0;

done:
    free(top);
    textstat_destroy(ts);
    return rc;
}

int main(int argc, char **argv)
{
    Options opt = { .top = 10, .min_len = 1, .json = 0 };

    static const struct option long_opts[] = {
        { "top",        required_argument, NULL, 't' },
        { "min-length", required_argument, NULL, 'm' },
        { "json",       no_argument,       NULL, 'j' },
        { "help",       no_argument,       NULL, 'h' },
        { "version",    no_argument,       NULL, 'V' },
        { NULL, 0, NULL, 0 }
    };

    int ch;
    while ((ch = getopt_long(argc, argv, "t:m:jhV", long_opts, NULL)) != -1) {
        switch (ch) {
        case 't':
            if (parse_size(optarg, &opt.top) != 0) {
                fprintf(stderr, PROG_NAME ": invalid value for --top: '%s'\n", optarg);
                return EXIT_USAGE;
            }
            break;
        case 'm':
            if (parse_size(optarg, &opt.min_len) != 0) {
                fprintf(stderr, PROG_NAME ": invalid value for --min-length: '%s'\n", optarg);
                return EXIT_USAGE;
            }
            break;
        case 'j':
            opt.json = 1;
            break;
        case 'h':
            usage(stdout);
            return EXIT_SUCCESS;
        case 'V':
            puts(PROG_NAME " " TEXTSTAT_VERSION);
            return EXIT_SUCCESS;
        default:
            usage(stderr);
            return EXIT_USAGE;
        }
    }

    int status = EXIT_SUCCESS;
    int json_first = 1;

    if (opt.json)
        fputs("[", stdout);

    if (optind >= argc) {
        if (process("<stdin>", stdin, &opt, &json_first) != 0)
            status = EXIT_RUNTIME;
    } else {
        for (int i = optind; i < argc; i++) {
            const char *name = argv[i];
            int from_stdin = strcmp(name, "-") == 0;
            FILE *fp = from_stdin ? stdin : fopen(name, "rb");

            if (fp == NULL) {
                fprintf(stderr, PROG_NAME ": %s: %s\n", name, strerror(errno));
                status = EXIT_RUNTIME;
                continue;
            }
            if (process(from_stdin ? "<stdin>" : name, fp, &opt, &json_first) != 0)
                status = EXIT_RUNTIME;
            if (!from_stdin)
                fclose(fp);
        }
    }

    if (opt.json)
        fputs(json_first ? "]\n" : "\n]\n", stdout);

    if (fflush(stdout) != 0 || ferror(stdout)) {
        fprintf(stderr, PROG_NAME ": write error: %s\n", strerror(errno));
        return EXIT_RUNTIME;
    }
    return status;
}
