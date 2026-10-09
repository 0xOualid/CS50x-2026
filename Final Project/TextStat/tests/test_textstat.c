/*
 * test_textstat.c - dependency-free unit tests for the textstat library.
 * Exits with status 0 if every check passes, 1 otherwise.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "textstat.h"

static int checks_run;
static int checks_failed;

#define CHECK(cond)                                                          \
    do {                                                                     \
        checks_run++;                                                        \
        if (!(cond)) {                                                       \
            checks_failed++;                                                 \
            fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);\
        }                                                                    \
    } while (0)

/* Analyze an in-memory string via a temporary file. */
static TextStats *analyze(const char *text, size_t min_len)
{
    TextStats *ts = textstat_create(min_len);
    FILE *fp = tmpfile();

    if (ts == NULL || fp == NULL) {
        fprintf(stderr, "test setup failed\n");
        exit(2);
    }
    fputs(text, fp);
    rewind(fp);
    if (textstat_read(ts, fp) != 0) {
        fprintf(stderr, "textstat_read failed\n");
        exit(2);
    }
    fclose(fp);
    return ts;
}

static void test_empty(void)
{
    TextStats *ts = analyze("", 1);
    TextSummary s;
    WordCount *top;
    size_t n;

    textstat_summary(ts, &s);
    CHECK(s.bytes == 0 && s.lines == 0 && s.words == 0);
    CHECK(s.unique_words == 0 && s.longest_line == 0);
    CHECK(textstat_top(ts, 5, &top, &n) == 0);
    CHECK(top == NULL && n == 0);
    textstat_destroy(ts);
}

static void test_basic_counts(void)
{
    const char *text = "Hello, world!\nHello again";
    TextStats *ts = analyze(text, 1);
    TextSummary s;
    WordCount *top;
    size_t n;

    textstat_summary(ts, &s);
    CHECK(s.bytes == strlen(text));
    CHECK(s.lines == 2);
    CHECK(s.words == 4);
    CHECK(s.unique_words == 3);

    CHECK(textstat_top(ts, 10, &top, &n) == 0);
    CHECK(n == 3);
    CHECK(strcmp(top[0].word, "hello") == 0 && top[0].count == 2);
    free(top);
    textstat_destroy(ts);
}

static void test_case_folding(void)
{
    TextStats *ts = analyze("Go GO go gO", 1);
    TextSummary s;

    textstat_summary(ts, &s);
    CHECK(s.words == 4);
    CHECK(s.unique_words == 1);
    textstat_destroy(ts);
}

static void test_apostrophes(void)
{
    TextStats *ts = analyze("Don't stop 'quoted' dogs' ''", 1);
    TextSummary s;
    WordCount *top;
    size_t n;
    int found_dont = 0, found_quoted = 0, found_dogs = 0;

    textstat_summary(ts, &s);
    CHECK(s.words == 4); /* don't, stop, quoted, dogs */

    CHECK(textstat_top(ts, 10, &top, &n) == 0);
    for (size_t i = 0; i < n; i++) {
        found_dont   |= strcmp(top[i].word, "don't") == 0;
        found_quoted |= strcmp(top[i].word, "quoted") == 0;
        found_dogs   |= strcmp(top[i].word, "dogs") == 0;
    }
    CHECK(found_dont && found_quoted && found_dogs);
    free(top);
    textstat_destroy(ts);
}

static void test_min_length(void)
{
    TextStats *ts = analyze("a an the cat", 3);
    TextSummary s;

    textstat_summary(ts, &s);
    CHECK(s.words == 4);        /* all tokens still counted ...           */
    CHECK(s.unique_words == 2); /* ... but only "the" and "cat" are kept  */
    textstat_destroy(ts);
}

static void test_ordering(void)
{
    TextStats *ts = analyze("b a b a c", 1);
    WordCount *top;
    size_t n;

    CHECK(textstat_top(ts, 2, &top, &n) == 0);
    CHECK(n == 2);                                  /* limited to top-2     */
    CHECK(strcmp(top[0].word, "a") == 0);           /* tie broken by A..Z   */
    CHECK(strcmp(top[1].word, "b") == 0);
    CHECK(top[0].count == 2 && top[1].count == 2);
    free(top);

    CHECK(textstat_top(ts, 0, &top, &n) == 0);      /* n == 0 -> nothing    */
    CHECK(top == NULL && n == 0);
    textstat_destroy(ts);
}

static void test_lines(void)
{
    TextSummary s;
    TextStats *ts;

    ts = analyze("abc\nabcdef\nab", 1);
    textstat_summary(ts, &s);
    CHECK(s.lines == 3 && s.longest_line == 6);
    textstat_destroy(ts);

    ts = analyze("x", 1);                 /* no trailing newline */
    textstat_summary(ts, &s);
    CHECK(s.lines == 1);
    textstat_destroy(ts);

    ts = analyze("x\n", 1);                /* with trailing newline */
    textstat_summary(ts, &s);
    CHECK(s.lines == 1);
    textstat_destroy(ts);

    ts = analyze("\n\n\n", 1);             /* blank lines */
    textstat_summary(ts, &s);
    CHECK(s.lines == 3 && s.words == 0);
    textstat_destroy(ts);
}

static void test_table_growth(void)
{
    enum { N = 5000 };
    TextStats *ts = textstat_create(1);
    FILE *fp = tmpfile();
    TextSummary s;
    WordCount *top;
    size_t n;

    CHECK(ts != NULL && fp != NULL);
    for (int i = 0; i < N; i++)
        fprintf(fp, "word%d ", i);
    fputs("word7 word7", fp);
    rewind(fp);

    CHECK(textstat_read(ts, fp) == 0);
    fclose(fp);

    textstat_summary(ts, &s);
    CHECK(s.words == N + 2);
    CHECK(s.unique_words == N);

    CHECK(textstat_top(ts, 1, &top, &n) == 0);
    CHECK(n == 1 && strcmp(top[0].word, "word7") == 0 && top[0].count == 3);
    free(top);
    textstat_destroy(ts);
}

static void test_long_word(void)
{
    char *text = malloc(10001);
    TextSummary s;
    TextStats *ts;

    CHECK(text != NULL);
    memset(text, 'x', 10000);
    text[10000] = '\0';

    ts = analyze(text, 1);
    textstat_summary(ts, &s);
    CHECK(s.words == 1 && s.unique_words == 1 && s.longest_line == 10000);
    textstat_destroy(ts);
    free(text);
}

static void test_accumulate(void)
{
    TextStats *ts = analyze("one two", 1);
    FILE *fp = tmpfile();
    TextSummary s;

    fputs("two three", fp);
    rewind(fp);
    CHECK(textstat_read(ts, fp) == 0);
    fclose(fp);

    textstat_summary(ts, &s);
    CHECK(s.words == 4 && s.unique_words == 3);
    textstat_destroy(ts);
}

int main(void)
{
    test_empty();
    test_basic_counts();
    test_case_folding();
    test_apostrophes();
    test_min_length();
    test_ordering();
    test_lines();
    test_table_growth();
    test_long_word();
    test_accumulate();

    textstat_destroy(NULL); /* must be a no-op */

    printf("%d checks, %d failed\n", checks_run, checks_failed);
    return checks_failed ? 1 : 0;
}
