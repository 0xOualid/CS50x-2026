/*
 * textstat.h - text statistics and word-frequency analysis.
 *
 * Usage:
 *     TextStats *ts = textstat_create(1);
 *     textstat_read(ts, fp);
 *     TextSummary s;
 *     textstat_summary(ts, &s);
 *     ...
 *     textstat_destroy(ts);
 *
 * A "word" is a run of ASCII letters and digits, optionally containing
 * apostrophes (so "don't" is one word). Words are lower-cased before counting.
 */
#ifndef TEXTSTAT_H
#define TEXTSTAT_H

#include <stddef.h>
#include <stdio.h>

#define TEXTSTAT_VERSION "1.0.0"

/* Opaque analyzer state. */
typedef struct TextStats TextStats;

/* Aggregate counters for everything read so far. */
typedef struct {
    size_t bytes;        /* total bytes read                               */
    size_t lines;        /* newline-terminated lines, plus a trailing
                            partial line if present                        */
    size_t words;        /* total word tokens                              */
    size_t unique_words; /* distinct words tracked (>= min length)         */
    size_t longest_line; /* length of the longest line, in bytes           */
} TextSummary;

/* One entry of the frequency table. `word` is owned by the TextStats. */
typedef struct {
    const char *word;
    size_t      count;
} WordCount;

/*
 * Create an analyzer. Words shorter than `min_word_len` are still counted
 * in the total word count but are left out of the frequency table.
 * Returns NULL on allocation failure.
 */
TextStats *textstat_create(size_t min_word_len);

/* Free an analyzer and everything it owns. Accepts NULL. */
void textstat_destroy(TextStats *ts);

/*
 * Consume `fp` until EOF, updating the statistics. May be called more than
 * once to accumulate across several streams.
 * Returns 0 on success, -1 on read error or allocation failure (errno is
 * set by the underlying failure where available).
 */
int textstat_read(TextStats *ts, FILE *fp);

/* Copy the current counters into `out`. */
void textstat_summary(const TextStats *ts, TextSummary *out);

/*
 * Build the top-`n` most frequent words, ordered by count (descending) and
 * then alphabetically. On success stores a newly allocated array in `*out`
 * (free() it; the `word` pointers stay valid until textstat_destroy) and its
 * length in `*out_len`. `*out` is NULL when the table is empty or n == 0.
 * Returns 0 on success, -1 on allocation failure.
 */
int textstat_top(const TextStats *ts, size_t n, WordCount **out, size_t *out_len);

#endif /* TEXTSTAT_H */
