/// Note: each state holds the live local variables.
enum State {
    Start { a: i32, b: i32 },
    State1 { b: i32, a2: i32 },  // Note: a is no longer useful.
    State2 { a2: i32, b2: i32 }, // Note: neither a nor b are useful now.
    End { result: i32 },
}

/// Calling this function merely gets the initial state.
/// It doesn't actually execute the body of the original function.
fn square_sum(a: i32, b: i32) -> State {
    State::Start { a, b }
}

/// Call this for each step.
fn square_sum_step(state: State) -> State {
    match state {
        State::Start { a, b } => { // Restore local variables from the state.
            let a2 = a * a;
            println!("a * a = {}", a2);
            State::State1 { b, a2 } // Save useful local variables into state.
        }

        State::State1 { b, a2 } => { // restore
            let b2 = b * b;
            println!("b * b = {}", b2);
            State::State2 { a2, b2 } // save
        }

        State::State2 { a2, b2 } => { // restore
            let result = a2 + b2;
            println!("result = {}", result);
            State::End { result } // save
        }

        State::End { .. } => {
            panic!("Coroutine already finished!");
        }
    }
}

fn main() {
    let mut state = square_sum(3, 4);
    loop {
        match state {
            State::End { result } => {
                println!("Execution finished. Result is {}", result);
                break;
            }
            _ => {
                println!("Resuming...");
                state = square_sum_step(state);
                println!("Yielded.");
            }
        }
    }
}
