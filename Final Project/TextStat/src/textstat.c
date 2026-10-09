/*
 * textstat.c - implementation of the text analysis library.
 *
 * Words are stored in an open-addressing hash table (FNV-1a, linear probing,
 * power-of-two capacity, 70% maximum load). Input is processed as a stream,
 * one byte at a time, so memory use depends on the number of distinct words
 * and not on the size of the input.
 */
#include "textstat.h"

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 64u
#define MAX_LOAD_NUM     7u
#define MAX_LOAD_DEN     10u
#define INITIAL_BUF_CAP  32u

typedef struct {
    char  *word; /* NULL marks an empty slot */
    size_t len;
    size_t count;
} Slot;

struct TextStats {
    size_t min_word_len;

    /* running counters */
    size_t bytes;
    size_t lines;
    size_t words;
    size_t longest_line;
    size_t line_len;

    /* frequency table */
    Slot  *slots;
    size_t capacity;
    size_t used;

    /* word being assembled */
    char  *buf;
    size_t buf_len;
    size_t buf_cap;
};

/* ---------------------------------------------------------------- table -- */

static uint64_t hash_bytes(const char *s, size_t n)
{
    uint64_t h = 14695981039346656037ULL; /* FNV-1a offset basis */
    for (size_t i = 0; i < n; i++) {
        h ^= (unsigned char)s[i];
        h *= 1099511628211ULL;             /* FNV prime */
    }
    return h;
}

/* Index of the slot holding `w`, or of the empty slot where it belongs. */
static size_t probe(const Slot *slots, size_t cap, const char *w, size_t n)
{
    size_t mask = cap - 1;
    size_t i = (size_t)hash_bytes(w, n) & mask;

    while (slots[i].word != NULL &&
           !(slots[i].len == n && memcmp(slots[i].word, w, n) == 0)) {
        i = (i + 1) & mask;
    }
    return i;
}

static int table_grow(TextStats *ts)
{
    size_t new_cap = ts->capacity ? ts->capacity * 2 : INITIAL_CAPACITY;
    Slot *fresh = calloc(new_cap, sizeof *fresh);
    if (fresh == NULL)
        return -1;

    for (size_t i = 0; i < ts->capacity; i++) {
        if (ts->slots[i].word != NULL) {
            size_t j = probe(fresh, new_cap, ts->slots[i].word, ts->slots[i].len);
            fresh[j] = ts->slots[i];
        }
    }
    free(ts->slots);
    ts->slots = fresh;
    ts->capacity = new_cap;
    return 0;
}

static int table_add(TextStats *ts, const char *w, size_t n)
{
    if ((ts->used + 1) * MAX_LOAD_DEN > ts->capacity * MAX_LOAD_NUM) {
        if (table_grow(ts) != 0)
            return -1;
    }

    size_t i = probe(ts->slots, ts->capacity, w, n);
    if (ts->slots[i].word != NULL) {
        ts->slots[i].count++;
        return 0;
    }

    char *copy = malloc(n + 1);
    if (copy == NULL)
        return -1;
    memcpy(copy, w, n);
    copy[n] = '\0';

    ts->slots[i].word  = copy;
    ts->slots[i].len   = n;
    ts->slots[i].count = 1;
    ts->used++;
    return 0;
}

/* --------------------------------------------------------------- parser -- */

static int buf_push(TextStats *ts, char c)
{
    if (ts->buf_len == ts->buf_cap) {
        size_t cap = ts->buf_cap ? ts->buf_cap * 2 : INITIAL_BUF_CAP;
        char *p = realloc(ts->buf, cap);
        if (p == NULL)
            return -1;
        ts->buf = p;
        ts->buf_cap = cap;
    }
    ts->buf[ts->buf_len++] = c;
    return 0;
}

/* Finish the word in progress (if any) and record it. */
static int flush_word(TextStats *ts)
{
    size_t n = ts->buf_len;
    ts->buf_len = 0;

    while (n > 0 && ts->buf[n - 1] == '\'') /* trim trailing apostrophes */
        n--;
    if (n == 0)
        return 0;

    ts->words++;
    if (n < ts->min_word_len)
        return 0;
    return table_add(ts, ts->buf, n);
}

static void close_line(TextStats *ts)
{
    ts->lines++;
    if (ts->line_len > ts->longest_line)
        ts->longest_line = ts->line_len;
    ts->line_len = 0;
}

/* ------------------------------------------------------------ public API -- */

TextStats *textstat_create(size_t min_word_len)
{
    TextStats *ts = calloc(1, sizeof *ts);
    if (ts == NULL)
        return NULL;
    ts->min_word_len = min_word_len;
    return ts;
}

void textstat_destroy(TextStats *ts)
{
    if (ts == NULL)
        return;
    for (size_t i = 0; i < ts->capacity; i++)
        free(ts->slots[i].word);
    free(ts->slots);
    free(ts->buf);
    free(ts);
}

int textstat_read(TextStats *ts, FILE *fp)
{
    int c;

    while ((c = getc(fp)) != EOF) {
        ts->bytes++;

        if (c == '\n') {
            if (flush_word(ts) != 0)
                return -1;
            close_line(ts);
            continue;
        }

        ts->line_len++;

        /* An apostrophe only continues a word; it never starts one. */
        if (isalnum(c) || (c == '\'' && ts->buf_len > 0)) {
            if (buf_push(ts, (char)tolower(c)) != 0)
                return -1;
        } else if (flush_word(ts) != 0) {
            return -1;
        }
    }

    if (flush_word(ts) != 0)
        return -1;
    if (ts->line_len > 0) /* final line without a trailing newline */
        close_line(ts);

    return ferror(fp) ? -1 : 0;
}

void textstat_summary(const TextStats *ts, TextSummary *out)
{
    out->bytes        = ts->bytes;
    out->lines        = ts->lines;
    out->words        = ts->words;
    out->unique_words = ts->used;
    out->longest_line = ts->longest_line;
}

static int compare_entries(const void *a, const void *b)
{
    const WordCount *x = a;
    const WordCount *y = b;

    if (x->count != y->count)
        return x->count > y->count ? -1 : 1;
    return strcmp(x->word, y->word);
}

int textstat_top(const TextStats *ts, size_t n, WordCount **out, size_t *out_len)
{
    *out = NULL;
    *out_len = 0;

    if (n == 0 || ts->used == 0)
        return 0;

    WordCount *all = malloc(ts->used * sizeof *all);
    if (all == NULL)
        return -1;

    size_t k = 0;
    for (size_t i = 0; i < ts->capacity; i++) {
        if (ts->slots[i].word != NULL) {
            all[k].word  = ts->slots[i].word;
            all[k].count = ts->slots[i].count;
            k++;
        }
    }
    qsort(all, k, sizeof *all, compare_entries);

    *out = all;
    *out_len = n < k ? n : k;
    return 0;
}
