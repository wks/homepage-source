#include <iostream>
#include <boost/coroutine2/coroutine.hpp>

struct Node {
    int value;
    Node *first_child;
    Node *next_sibling;
};

// [1, [[2, 3], [4, 5]], [6, 7, 8]]
Node node8        = { .value = 8,  .first_child = nullptr, .next_sibling = nullptr   };
Node node7        = { .value = 7,  .first_child = nullptr, .next_sibling = &node8    };
Node node6        = { .value = 6,  .first_child = nullptr, .next_sibling = &node7    };
Node node678      = { .value = -1, .first_child = &node6,  .next_sibling = nullptr   };
Node node5        = { .value = 5,  .first_child = nullptr, .next_sibling = nullptr   };
Node node4        = { .value = 4,  .first_child = nullptr, .next_sibling = &node5    };
Node node45       = { .value = -1, .first_child = &node4,  .next_sibling = nullptr   };
Node node3        = { .value = 3,  .first_child = nullptr, .next_sibling = nullptr   };
Node node2        = { .value = 2,  .first_child = nullptr, .next_sibling = &node3    };
Node node23       = { .value = -1, .first_child = &node2,  .next_sibling = &node45   };
Node node2345     = { .value = -1, .first_child = &node23, .next_sibling = &node678  };
Node node1        = { .value = 1,  .first_child = nullptr, .next_sibling = &node2345 };
Node node12345678 = { .value = -1, .first_child = &node1,  .next_sibling = nullptr   };

struct State {
    int current_value = -1;
    bool finished = false;
};

void visit_node(Node *node, State &state, boost::context::fiber &sink) {
    for (Node *current = node; current != nullptr; current = current->next_sibling) {
        if (current->first_child == nullptr) {
            state.current_value = current->value;
            sink = std::move(sink).resume();   // Yield at any level.  It's stackful!
        } else {
            visit_node(current->first_child, state, sink);   // Recursive call.
        }
    }
}

int main() {
    State state;

    auto traverser = boost::context::fiber([&state](boost::context::fiber &&sink) {
        visit_node(&node12345678, state, sink);
        state.finished = true;
        return std::move(sink);
    });

    while (true) {
        traverser = std::move(traverser).resume();  // Resume the coroutine.
        if (state.finished) {
            break;
        }
        std::cout << state.current_value << std::endl;
    }

    return 0;
}

