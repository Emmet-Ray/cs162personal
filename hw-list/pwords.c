/*
 * Word count application with one thread per input file.
 *
 * You may modify this file in any way you like, and are expected to modify it.
 * Your solution must read each input file from a separate thread. We encourage
 * you to make as few changes as necessary.
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

#define MAX_WORD_LEN 64

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <pthread.h>

#include "word_count.h"
#include "word_helpers.h"

word_count_list_t word_counts;

void* thread_per_file(void* file) {
  char* file_name = (char*)file; 
  printf("file : %s\n", file_name);

  FILE* infile = fopen(file_name, "r");
  char word[MAX_WORD_LEN]; 
  int word_len = 0;
  int c;
  while((c = fgetc(infile)) != EOF) {
      if(isalpha(c)) {
        word[word_len++] = tolower(c);
      } else if (word_len >= 2) {  
        word[word_len] = '\0';
        //printf("%s\n", word);
        add_word(&word_counts, word);
        word_len = 0;
      }
  }
  //printf("<1>\n");
  if (word_len >= 2) {
    word[word_len] = '\0';
    add_word(&word_counts, word);
  }
  pthread_exit(NULL);
}
/*
 * main - handle command line, spawning one thread per file.
 */
int main(int argc, char* argv[]) {
  /* Create the empty data structure. */
  init_words(&word_counts);
  if (pthread_mutex_init(&word_counts.lock, NULL)) {
      printf("ERROR : lock init failed\n");
  }
  if (argc <= 1) {
    /* Process stdin in a single thread. */
    count_words(&word_counts, stdin);
  } else {
    /* TODO */
    int num_threads = argc - 1;
    printf("num_threads : %d\n", num_threads);
    int create_code;
    pthread_t threads[num_threads]; 
    for (int i = 0; i < num_threads; i++) {
      create_code = pthread_create(&threads[i], NULL, thread_per_file, argv[1 + i]);
      printf("create thread %d\n", i);
      if (create_code) {
        printf("ERROR; return code from pthread_create() is %d\n", create_code);
        exit(-1);
      }
    }
    
    for (int i = 0; i < num_threads; i++) {
      pthread_join(threads[i], NULL);
    }
    pthread_mutex_destroy(&word_counts.lock);
  }
  
  /* Output final result of all threads' work. */
  wordcount_sort(&word_counts, less_count);
  fprint_words(&word_counts, stdout);
  return 0;
}
