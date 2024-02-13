#include "token.h"

#include <stdlib.h>

void token_free(struct token token)
{
    free(token.value);

    // free the expansions
    if (token.first)
    {
        struct expansion *head = token.first;
        while (head->next)
        {
            struct expansion *next = head->next;
            free(head->value);
            free(head);
            head = next;
        }

        free(head->value);
        free(head);
    }
}
