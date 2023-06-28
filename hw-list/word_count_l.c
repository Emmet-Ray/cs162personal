/*
 * Implementation of the word_count interface using Pintos lists.
 *
 * You may modify this file, and are expected to modify it.
 */

/*
 * Copyright © 2021 University of California, Berkeley
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PINTOS_LIST
#define PINTOS_LIST
#error "PINTOS_LIST must be #define'd when compiling word_count_l.c"
#endif

#include "word_count.h"

// wclist is the pointer to the created struct list 
void init_words(word_count_list_t* wclist) { /* TODO */
  list_init(wclist);
}

size_t len_words(word_count_list_t* wclist) {
  int cnt = 0;
  struct list_elem *p;
  for (p = list_begin(wclist); p != list_end(wclist); p = list_next(p)) {
    cnt++;
  }
  return cnt;
}

word_count_t* find_word(word_count_list_t* wclist, char* word) {
  struct list_elem *p;
  word_count_t *tmp;
  for (p = list_begin(wclist); p != list_end(wclist); p = list_next(p)) {
    tmp = list_entry(p, struct word_count, elem); 
    if (!strcmp(word, tmp->word)) {
      return tmp;
    }
  }
  return NULL;
}

word_count_t* add_word(word_count_list_t* wclist, char* word) {
  word_count_t *tmp;
  if ((tmp = find_word(wclist, word)) != NULL) {
    tmp->count++;
    return tmp;
  }
  tmp = (word_count_t *)malloc(sizeof(word_count_t));
  tmp->count = 1;
  strcpy(tmp->word, word);
  list_push_front(wclist, &tmp->elem);  
  return tmp;
}

void fprint_words(word_count_list_t* wclist, FILE* outfile) { /* TODO */
  struct list_elem *p;
  word_count_t *tmp;
  for (p = list_begin(wclist); p != list_end(wclist); p = list_next(p)) {
    tmp = list_entry(p, word_count_t, elem); 
    fprintf(outfile, "%i\t%s\n", tmp->count, tmp->word);
  }
}

static bool less_list(const struct list_elem* ewc1, const struct list_elem* ewc2, void* aux) {
  /* TODO */
  word_count_t *tmp1 = list_entry(ewc1, word_count_t, elem);
  word_count_t *tmp2 = list_entry(ewc2, word_count_t, elem);
  bool (*less)(word_count_t*, word_count_t*) = aux;
  return (*less)(tmp1, tmp2);
}

void wordcount_sort(word_count_list_t* wclist,
                    bool less(const word_count_t*, const word_count_t*)) {
  list_sort(wclist, less_list, less);
}
