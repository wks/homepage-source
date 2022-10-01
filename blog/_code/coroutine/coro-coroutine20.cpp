#include <iostream>
#include <coroutine>

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

struct Traverser {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
        int current_value_ = -1;
        bool finished_ = false;

        // This is executed when the "coroutine" is created.
        Traverser get_return_object() {
            return Traverser(handle_type::from_promise(*this));
        }

        // Called at the beginning of the coroutine.
        // We let it stop there to mimic Python generator behaviour.
        std::suspend_always initial_suspend() {
            return {};
        }

        // Called when the coroutine finished execution.
        std::suspend_always final_suspend() noexcept {
            // We set a variable so the main function knows it finished.
            finished_ = true;
            return {};
        }

        // Called when a co_yield expression is evaluated.
        std::suspend_always yield_value(int value) {
            current_value_ = value;
            return {};
        }

        // Called when an exception is thrown.
        void unhandled_exception() {
            std::terminate();
        }
    };

    handle_type handle_;

    Traverser(handle_type handle) : handle_(handle) {}

    void resume() {
        handle_.resume();
    }

    bool finished() const {
        return handle_.promise().finished_;
    }

    int get_value() {
        return handle_.promise().current_value_;
    }
};

Traverser visit_node(Node *node) {
    for (Node *current = node; current != nullptr; current = current->next_sibling) {
        if (current->first_child == nullptr) {
            // Yield.
            co_yield current->value;
        } else {
            // It is stackless.  We need multiple levels of coroutines.
            auto sub_traverser = visit_node(current->first_child);
            while (true) {
                sub_traverser.resume();
                if (sub_traverser.finished()) {
                    break;
                }
                // Yield from sub-coroutine.
                co_yield sub_traverser.get_value();
            }
        }
    }
}

int main() {
    auto traverser = visit_node(&node12345678);
    while (true) {
        traverser.resume();
        if (traverser.finished()) {
            break;
        }
        std::cout << traverser.get_value() << std::endl;
    }

    return 0;
}

