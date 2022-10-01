use context::stack::ProtectedFixedSizeStack;
use context::{Context, Transfer};

#[derive(Debug)]
enum NestedList {
    Leaf(i32),
    Nested(Vec<NestedList>),
}

fn traverse(list: &NestedList, t: &mut Option<Transfer>) {
    match list {
        NestedList::Leaf(num) => {
            // Context-switching consumes the `Context` object.  We have to take and replace it.
            let old_t = t.take().unwrap();
            let new_t = unsafe { old_t.context.resume(*num as usize) };
            *t = Some(new_t);
        }
        NestedList::Nested(lists) => {
            for elem in lists {
                // This is stackful coroutine.  We can do recursive function call.
                traverse(elem, t)
            }
        }
    }
}

fn main() {
    // [1, [[2, 3], [4, 5]], [6, 7, 8]]
    let nested_list = NestedList::Nested(vec![
        NestedList::Leaf(1),
        NestedList::Nested(vec![
            NestedList::Nested(vec![NestedList::Leaf(2), NestedList::Leaf(3)]),
            NestedList::Nested(vec![NestedList::Leaf(4), NestedList::Leaf(5)]),
        ]),
        NestedList::Nested(vec![
            NestedList::Leaf(6),
            NestedList::Leaf(7),
            NestedList::Leaf(8),
        ]),
    ]);

    extern "C" fn context_function(t: Transfer) -> ! {
        // The initial Transfer carries the pointer to the list as `data`.
        let list = unsafe { &*(t.data as *const NestedList) };

        // From now on, we'll frequently take and replace the Transfer. Use Option.
        let mut t_holder = Some(t);
        traverse(list, &mut t_holder);

        // Send a special value to indicate the end of traversal.
        let t = t_holder.unwrap();
        unsafe { t.context.resume(usize::MAX) };
        unreachable!();
    }

    let stack = ProtectedFixedSizeStack::default();

    let mut t = Transfer::new(unsafe { Context::new(&stack, context_function) }, 0);

    // The initial `resume` sends the list reference as a usize.
    t = unsafe { t.context.resume(&nested_list as *const NestedList as usize) };

    // Use this special value to indicate end of traversal.
    while t.data != usize::MAX {
        println!("{}", t.data);

        // Subsequent `resume` doesn't need to carry values.
        t = unsafe { t.context.resume(0usize) };
    }
}
