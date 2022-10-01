use async_recursion::async_recursion;
use std::future::Future;
use std::pin::Pin;
use std::sync::atomic::{AtomicBool, AtomicI32, Ordering};
use std::task::{Context, Poll};

#[derive(Default)]
struct ResultReporter {
    num: AtomicI32, // Rust is having problem figuring out whether the coroutine races with main,
    result_taken: AtomicBool, // so we just use atomic variables here.
}

impl ResultReporter {
    fn report_result<'a>(&'a self, num: i32) -> WaitUntilResultTaken<'a> {
        self.num.store(num, Ordering::Relaxed);
        self.result_taken.store(false, Ordering::Relaxed);
        WaitUntilResultTaken {
            result_taken: &self.result_taken,
        }
    }
    fn take_result(&self) -> i32 {
        let num = self.num.load(Ordering::Relaxed);
        self.result_taken.store(true, Ordering::Relaxed);
        num
    }
}

struct WaitUntilResultTaken<'a> {
    result_taken: &'a AtomicBool,
}

impl<'a> Future for WaitUntilResultTaken<'a> {
    type Output = ();

    fn poll(self: Pin<&mut Self>, _: &mut Context<'_>) -> Poll<Self::Output> {
        if self.result_taken.load(Ordering::Relaxed) {
            Poll::Ready(())
        } else {
            Poll::Pending
        }
    }
}

enum NestedList {
    Leaf(i32),
    Nested(Vec<NestedList>),
}

#[async_recursion]
async fn traverse(list: &NestedList, reporter: &ResultReporter) {
    match list {
        NestedList::Leaf(num) => {
            // This `await` expression will call `WaitUntilResultTaken::poll()` twice.
            // The first time is when we reach the `await` here.  It returns `Poll::Pending` so we yield.
            // The second time is when `main` calls `poll`.  It returns `Poll::Ready(())` so we continue.
            reporter.report_result(*num).await
        }
        NestedList::Nested(lists) => {
            for elem in lists {
                // This `await` will pass the `Poll::Pending` to the caller level by level until it reaches `main`.
                traverse(elem, reporter).await
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

    let result_reporter = ResultReporter::default();

    let mut coro = traverse(&nested_list, &result_reporter);

    let null_ctx = unsafe { &mut *std::ptr::null_mut::<Context>() };

    loop {
        let coro_p = unsafe { Pin::new_unchecked(&mut coro) };
        let poll_result = coro_p.poll(null_ctx); // Let the coroutine run.
        match poll_result {
            Poll::Pending => {
                // When pausing (at `await` sites) during execution, we get the result.
                let num = result_reporter.take_result();
                println!("{}", num);
            }
            Poll::Ready(()) => {
                // When finished, we just quit.
                break;
            }
        }
    }
}
