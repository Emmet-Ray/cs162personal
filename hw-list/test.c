#include "word_count.h"
#include <assert.h>

int main(int argc, char const *argv[])
{
    word_count_list_t* lst;
    init_words(lst);
    assert(len_words(lst) == 0);    
    return 0;
}
