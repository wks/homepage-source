#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

struct Node {
    int value;
    struct Node *first_child;
    struct Node *next_sibling;
};

// [1, [[2, 3], [4, 5]], [6, 7, 8]]
struct Node node8        = { .value = 8,  .first_child = NULL,    .next_sibling = NULL      };
struct Node node7        = { .value = 7,  .first_child = NULL,    .next_sibling = &node8    };
struct Node node6        = { .value = 6,  .first_child = NULL,    .next_sibling = &node7    };
struct Node node678      = { .value = -1, .first_child = &node6,  .next_sibling = NULL      };
struct Node node5        = { .value = 5,  .first_child = NULL,    .next_sibling = NULL      };
struct Node node4        = { .value = 4,  .first_child = NULL,    .next_sibling = &node5    };
struct Node node45       = { .value = -1, .first_child = &node4,  .next_sibling = NULL      };
struct Node node3        = { .value = 3,  .first_child = NULL,    .next_sibling = NULL      };
struct Node node2        = { .value = 2,  .first_child = NULL,    .next_sibling = &node3    };
struct Node node23       = { .value = -1, .first_child = &node2,  .next_sibling = &node45   };
struct Node node2345     = { .value = -1, .first_child = &node23, .next_sibling = &node678  };
struct Node node1        = { .value = 1,  .first_child = NULL,    .next_sibling = &node2345 };
struct Node node12345678 = { .value = -1, .first_child = &node1,  .next_sibling = NULL      };

ucontext_t main_context;
ucontext_t coro_context;
int current_value;
bool finished;

void do_yield(int value) {
    current_value = value;
    swapcontext(&coro_context, &main_context);  // switch back to main
}

void visit_node(struct Node *node) {
    for (struct Node *current = node; current != NULL; current = current->next_sibling) {
        if (current->first_child == NULL) {
            do_yield(current->value);
        } else {
            visit_node(current->first_child);   // recursive call
        }
    }
}

void traverse() {
    finished = false;
    visit_node(&node12345678);
    finished = true;
}

int main() {
    getcontext(&coro_context);

    // Obviously, it is stackful.
    void *stack = malloc(65536);    // Not sure if 65536 is enough, though.
                                    // If we are careful enough, we should use mprotect
                                    // to create a PROT_NONE region to protect against
                                    // stack overflow.
    coro_context.uc_stack = (stack_t){ .ss_sp = stack, .ss_size = 4096 };
    coro_context.uc_link = &main_context;
    makecontext(&coro_context, (void(*)())traverse, 0);

    while (true) {
        swapcontext(&main_context, &coro_context);  // switch to coroutine
        if (finished) {
            break;
        } else {
            printf("%d\n", current_value);
        }
    }

    return 0;
}

