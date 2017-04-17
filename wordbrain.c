#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void usage() {
  printf("please specify your puzzle as your argument as nine characters reading left-to-right, top-to-bottom, followed by the lengths of your words\n");
  exit(0);
}

typedef struct LWord {
  char *word;
  size_t len;
  struct LWord *next;
} LWord;

static char * spaces = "                                             ";
static char * indentation;

static void ppuzzle(char *puzzle, int w, int h) {
  for (int y=0; y<h; y++) {
    printf("%s", indentation);
    for (int x=0; x<w; x++) {
      putchar(puzzle[x+y*w]);
    }
    putchar('\n');
  }
}

static char * fall(char *puzzle_in, int w, int h) {
  char *p = malloc(w*h);
  memcpy(p, puzzle_in, w*h);
  //printf("falling\n");
  //ppuzzle(p, w, h);
  for (int x=0; x<w; x++) {
    int yr, yw = h-1;
    while (1) {
      /* writer climbs to find an empty space */
      while (yw >= 1 && p[x+yw*w] != ' ')
        yw--;
      if (yw == 0)
        goto colcomplete;
      /* reader climbs to find a non-empty space */
      yr = yw-1;
      while (yr >= 0 && p[x+yr*w] == ' ')
        yr--;
      if (yr < 0)
        goto colcomplete;
      /* swap */
      char c = p[x+yw*w];
      p[x+yw*w] = p[x+yr*w];
      p[x+yr*w] = c;
    }
    colcomplete:
      {}
  }
  //printf("fallen\n");
  //ppuzzle(p, w, h);
  return p;
}

/* "Mutable" is a poor word to describe state that we will modify at entry- and exit-time in our recursive solution algorithm. This struct doesn't need to be copied, it's passed by reference and modified as needed (or in the cases of some fields, never modified.) */
typedef struct MutableState {
  char *puzzle;
  char *solution;
  char *wordHead;
  char *wordTail;
  size_t num_words_remaining;
  size_t *word_lengths_remaining;
  size_t num_words_requested;
  int puzzle_w, puzzle_h;
} MutableState;

/* "Immutable" is a poor word for state which we need to save on the stack at each step in our recursive algorithm. This struct is passed by value and copied every call. */
static LWord *wordListHead;

static void solve(MutableState *ms, int x, int y) {
  /* Is this a valid move? */
  if (x < 0 || x >= ms->puzzle_w) return;
  if (y < 0 || y >= ms->puzzle_w) return;
  size_t xy = x + y * ms->puzzle_w;
  if (ms->puzzle[xy] == ' ') return;

  /* Mutate entry state */
  char character = ms->puzzle[xy];
  ms->puzzle[xy] = ' ';
  *(ms->wordTail++) = character;

          indentation--;

  /* Check for solutions */
  /* Right now we just do a linear search */
  LWord *word = wordListHead;
  int word_len = ms->wordTail - ms->wordHead;
  int partial_words_found = 0;
  do {
    if (strncmp(ms->wordHead, word->word, word_len) == 0) {
      if (word_len == word->len) {
        int word_len_index;
        for (word_len_index=0; word_len_index<ms->num_words_requested; word_len_index++) {
          if (ms->word_lengths_remaining[word_len_index] == word_len)
            break;
        }
        if (word_len_index < ms->num_words_requested) {
          /* Mutate entry state */
          ms->word_lengths_remaining[word_len_index] = 0;
          char* puzzle = ms->puzzle;
          ms->puzzle = fall(ms->puzzle, ms->puzzle_w, ms->puzzle_h);
          ms->num_words_remaining--;
          char * wordHead = ms->wordHead;
          char * wordTail = ms->wordTail;
          ms->wordHead = ms->wordTail;

          /* Is this a valid solution? */
          if (ms->num_words_remaining == 0) {
            printf("%ssolution: %s\n", indentation, ms->solution);
          } else {
            /* Recurse */
            for (int x2=0; x2<ms->puzzle_w; x2++)
            for (int y2=0; y2<ms->puzzle_h; y2++)
              solve(ms, x2, y2);
          }

          /* Unmutate exit state */
          ms->word_lengths_remaining[word_len_index] = word_len;
          free(ms->puzzle);
          ms->puzzle = puzzle;
          ms->num_words_remaining++;
          ms->wordHead = wordHead;
          ms->wordTail = wordTail;
        }
      }
      else {
        //printf("partial match\n");
        partial_words_found = 1;
      }
    }
  } while ((word = word->next) != 0);

  /* Recurse */
  if (partial_words_found) {
    solve(ms, x+1, y+1);
    solve(ms, x+1, y+0);
    solve(ms, x+1, y-1);
    solve(ms, x+0, y+1);
    solve(ms, x+0, y-1);
    solve(ms, x-1, y+1);
    solve(ms, x-1, y+0);
    solve(ms, x-1, y-1);
  }

  /* Unmutate exit state */
  ms->puzzle[xy] = character;
  *(--ms->wordTail) = '\0';

          indentation++;
}

size_t fixWord(char *buf) {
  char *read = buf;
  size_t len = 0;
  while (*read) {
    if (isalpha(*read)) {
      *(buf++) = tolower(*read);
      len++;
    }
    read++;
  }
  *buf = '\0';
  return len;
}

int main(int argc, char **argv) {
  indentation = &spaces[45];

  MutableState ms;
  size_t puzzle_len = strlen(argv[1]);

  switch (puzzle_len) {
    case 9:
      ms.puzzle_w = 3;
      ms.puzzle_h = 3;
      break;
    case 16:
      ms.puzzle_w = 4;
      ms.puzzle_h = 4;
      break;
    default:
      usage();
  }

  ms.puzzle = strdup(argv[1]);

  ms.num_words_remaining =
  ms.num_words_requested = argc-2;
  ms.word_lengths_remaining = malloc(sizeof(size_t) * ms.num_words_requested);
  for (size_t i=2; i<argc; i++)
    ms.word_lengths_remaining[i-2] = (size_t) atoi(argv[i]);

  ms.wordHead =
  ms.wordTail =
  ms.solution = malloc(ms.puzzle_w * ms.puzzle_h + ms.num_words_requested);
  memset(ms.solution, 0, ms.puzzle_w * ms.puzzle_h + 1);

  int letterFreqs[26];
  memset(letterFreqs, 0, sizeof(letterFreqs));

  for (char *c = ms.puzzle; *c; c++) {
    if (!isalpha(*c))
      usage();
    letterFreqs[tolower(*c)-'a'] ++;
  }

  FILE *f = fopen("/usr/share/dict/words", "r");

  wordListHead = 0;
  LWord **wordListTail;
  wordListTail = &wordListHead;

  size_t num_words_loaded = 0;
  char buf[4096];
  while (fgets(buf, sizeof(buf), f)) {
    size_t numLetters = fixWord(buf);

    for (size_t i=0; i<ms.num_words_remaining; i++)
      if (numLetters == ms.word_lengths_remaining[i])
        goto wordLenAccepted;
    goto lineDone;

    int tempLetterFreqs[26];

    wordLenAccepted:

    memcpy(tempLetterFreqs, letterFreqs, sizeof(letterFreqs));
    for (char *c = buf; *c; c++)
      if (--tempLetterFreqs[tolower(*c)-'a'] < 0)
        goto lineDone;

    *wordListTail = malloc(sizeof(LWord));
    (*wordListTail)->word = strdup(buf);
    (*wordListTail)->len = numLetters;
    (*wordListTail)->next = 0;
    //printf("saving word \"%s\"\n", (*wordListTail)->word);
    wordListTail = &(*wordListTail)->next;
    num_words_loaded++;

    lineDone:
    {}
  }

  printf("loaded %d words\n", (int) num_words_loaded);
  ppuzzle(ms.puzzle, ms.puzzle_w, ms.puzzle_h);
  for (int x=0; x<ms.puzzle_w; x++)
  for (int y=0; y<ms.puzzle_h; y++)
    solve(&ms, x, y);

  return 0;
}
