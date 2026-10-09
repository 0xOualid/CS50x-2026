# textstat

A small, fast command-line tool written in C11 that analyzes text. It reports
bytes, lines and words, and ranks the most frequent words, from files or
standard input, as plain text or JSON.

- **Streaming:** memory use depends on the number of *distinct* words, not the
  size of the input.
- **Zero dependencies:** just a C11 compiler and `make`.
- **Scriptable:** JSON output and well-defined exit codes.
- **Tested:** unit tests plus an AddressSanitizer/UBSan build target.

## Quick start

```sh
make            # build  -> build/textstat
make test       # run the unit tests
./build/textstat README.md
```

## Requirements

- A C11 compiler (GCC or Clang)
- GNU Make (or compatible)
- A POSIX-like system (Linux, macOS, BSD) for `getopt_long`

## Build and install

| Command          | What it does                                         |
|------------------|------------------------------------------------------|
| `make`           | Build `build/textstat`                               |
| `make test`      | Build and run the unit tests                         |
| `make sanitize`  | Run the tests under AddressSanitizer + UBSan         |
| `make install`   | Install to `/usr/local/bin` (`PREFIX=` and `DESTDIR=` are honored) |
| `make uninstall` | Remove the installed binary                          |
| `make clean`     | Remove build artifacts                               |
| `make help`      | List targets                                         |

Install somewhere else:

```sh
make install PREFIX=$HOME/.local
```

### Example session

A real run on Linux: build and install, check the help, then analyze the
project's own README.

```console
$ cd TextStat

$ sudo make install
mkdir -p build
cc -Iinclude -MMD -MP -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wstrict-prototypes -O2 -c src/main.c -o build/main.o
cc -Iinclude -MMD -MP -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wstrict-prototypes -O2 -c src/textstat.c -o build/textstat.o
cc build/main.o build/textstat.o  -o build/textstat
mkdir -p /usr/local/bin
install -m 755 build/textstat /usr/local/bin/textstat

$ textstat --help
Usage: textstat [OPTIONS] [FILE...]

Count bytes, lines and words, and list the most frequent words.
Reads standard input when no FILE is given or when FILE is '-'.

Options:
  -t, --top N          show the N most frequent words (default 10, 0 = none)
  -m, --min-length N   ignore words shorter than N in the frequency list
                       (default 1)
  -j, --json           print results as JSON
  -h, --help           show this help and exit
  -V, --version        show version and exit

Exit status: 0 success, 1 file or runtime error, 2 usage error.

$ textstat -t 5 README.md
== README.md ==
Bytes:         6130
Lines:         205
Words:         753
Unique words:  341
Longest line:  94

Top 5 words:
   1. and                  24
   2. words                23
   3. textstat             22
   4. the                  20
   5. a                    18
```

## Usage

```
textstat [OPTIONS] [FILE...]
```

With no `FILE`, or with `-` as a file name, `textstat` reads standard input.
Each file gets its own report.

| Option                | Description                                                        |
|-----------------------|--------------------------------------------------------------------|
| `-t, --top N`         | Show the N most frequent words (default `10`; `0` disables)        |
| `-m, --min-length N`  | Ignore words shorter than N in the frequency list (default `1`)    |
| `-j, --json`          | Print results as JSON                                              |
| `-h, --help`          | Show help and exit                                                 |
| `-V, --version`       | Show version and exit                                              |

### Examples

Analyze a file:

```console
$ textstat --top 5 sample.txt
== sample.txt ==
Bytes:         143
Lines:         3
Words:         33
Unique words:  16
Longest line:  61

Top 5 words:
   1. the                  5
   2. a                    4
   3. dog                  4
   4. fox                  4
   5. and                  2
```

Read from a pipe:

```console
$ printf 'hello hello world' | textstat --top 2
== <stdin> ==
Bytes:         17
Lines:         1
Words:         3
Unique words:  2
Longest line:  17

Top 2 words:
   1. hello                2
   2. world                1
```

Ignore short words and emit JSON:

```console
$ textstat -t 3 -m 4 --json sample.txt
[
  {
    "file": "sample.txt",
    "bytes": 143,
    "lines": 3,
    "words": 33,
    "unique_words": 8,
    "longest_line": 61,
    "top_words": [
      {"word": "quick", "count": 2},
      {"word": "brown", "count": 1},
      {"word": "didn't", "count": 1}
    ]
  }
]
```

Combine with other tools:

```sh
textstat --json *.md | jq '.[] | {file, words}'
curl -s https://example.com | textstat --top 20 -m 5
```

### Exit status

| Code | Meaning                                                   |
|------|-----------------------------------------------------------|
| `0`  | Success                                                   |
| `1`  | A file could not be opened or read, or memory ran out     |
| `2`  | Invalid command-line usage                                |

When several files are given, `textstat` reports on each readable file and
still exits with `1` if any file failed.

## How words and lines are counted

- A **word** is a run of ASCII letters and digits. An apostrophe inside a word
  is kept (`don't` is one word), and trailing apostrophes are trimmed.
- Words are **lower-cased**, so `Go`, `GO` and `go` are the same word.
- Non-ASCII bytes (for example UTF-8 accented letters) act as separators.
- A **line** ends at `\n`. A final line without a trailing newline still counts,
  so this differs slightly from `wc -l`.
- `--min-length` only affects the frequency list. The `Words` total includes
  every word, and `Unique words` counts distinct words that meet the minimum.
- Ties in the ranking are broken alphabetically, so output is deterministic.

## Project layout

```
textstat/
├── include/textstat.h      public library API
├── src/textstat.c          analyzer: streaming parser + hash table
├── src/main.c              command-line interface
├── tests/test_textstat.c   dependency-free unit tests
├── Makefile
├── LICENSE
└── README.md
```

The analysis code is a separate library from the CLI, so it can be reused in
other programs:

```c
#include "textstat.h"

TextStats *ts = textstat_create(1);
textstat_read(ts, fp);

TextSummary s;
textstat_summary(ts, &s);

WordCount *top; size_t n;
textstat_top(ts, 10, &top, &n);
/* ... use top[0..n-1] ... */
free(top);
textstat_destroy(ts);
```

## Design notes

- **Hash table:** open addressing with FNV-1a hashing, linear probing,
  power-of-two capacity and a 70% maximum load factor, so counting is O(1)
  amortized per word.
- **Error handling:** every allocation and I/O result is checked. Failures are
  reported with the file name and a `strerror` message on stderr.
- **Warnings:** built with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`
  and compiles cleanly.

## Testing

```sh
make test       # 39 checks
make sanitize   # same tests with AddressSanitizer and UBSan
```

The tests cover empty input, case folding, apostrophes, minimum length,
ranking and tie-breaking, line counting edge cases, table growth past
5,000 distinct words, very long words, and accumulating several streams.

## License

MIT. See [LICENSE](LICENSE).
