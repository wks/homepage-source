---
layout: post
title: "Flatten lists with coroutines, Rosetta Code style"
tags: coroutine,rosettacode
excerpt_separator: <!--more-->
---

I'll try to use **coroutines** to flatten nested lists, Rosetta Code stlye. That
means I'll do it in many different programming languages and libraries,
including Ruby, Lua, Python (including greenlets), etc.  This task shows the
difference between *symmetric* vs *asymmetric* coroutines, and *stackful* vs
*stackless* coroutines.

Note that this post alone may not be enough to teach you how to use coroutines
in all those languages.

<!--more-->


# The task

**Given**:

-   a nested list of numbers, such as `[1, [[2, 3], [4, 5]], [6, 7, 8]]`

**Enumerate**:

-   all numbers in the list, in the order they appear in the flattened list.

    When given the list above, the output should be 1, 2, 3, 4, 5, 6, 7 and
    8, in that order.

**Requirement**:

-   Use coroutine(s) to do the enumeration.

I will try to do this task using as many programming languages as possible,
[Rosetta Code][rosetta-code] style, to compare their coroutine syntax and API.
At the time of writing (2022), different programming languages still differ
greatly w.r.t. the design of coroutines.

[rosetta-code]: https://rosettacode.org/wiki/Rosetta_Code


# The code

## Ruby fibers (stackful, both asymmetric and symmetric)

[Fibers][ruby-fiber] are "primitives for implementing light weight cooperative
concurrency in Ruby".

Ruby fibers are stackful.  According to the [doc][ruby-fiber], "each fiber comes
with a stack. This enables the fiber to be paused from deeply nested function
calls within the fiber block".

Ruby fibers can operate in both asymmetric and symmetric mode.  I'll demonstrate
the task in both modes below.

[ruby-fiber]: https://ruby-doc.org/core-3.1.2/Fiber.html

### Asymmetric

The [`Fiber#resume`] instance method resumes a fiber, and a subsequent call to
the [`Fiber.yield`] class method jumps back to the resumer.

[`Fiber#resume`]: https://ruby-doc.org/core-3.1.2/Fiber.html#method-i-resume
[`Fiber::yield`]: https://ruby-doc.org/core-3.1.2/Fiber.html#method-c-yield

{% highlight ruby linenos %}
{% include_file blog/_code/coroutine/coro-a.rb %}
{% endhighlight %}

However, it is more natural to use the [`Enumerator`][ruby-enumerator] class to
automatically transform block-based call-back function into a fiber-based
coroutine.  It only uses fiber when the enumerator is used as an external
iterator (calling `e.next` repeatedly).  It still uses call-back for internal
iteration (`e.each { |v| ... }`).

[ruby-enumerator]: https://ruby-doc.org/core-3.1.2/Enumerator.html

{% highlight ruby linenos %}
{% include_file blog/_code/coroutine/coro-a-enum.rb %}
{% endhighlight %}

### Symmetric

The [`Fiber#transfer`] method can switch to any fiber, but always needs an
explicit fiber to switch to.  We can pass the current fiber to the new fiber
when we create it, so it can remember which fiber to transfer back to.

[`Fiber#transfer`]: https://ruby-doc.org/core-3.1.2/Fiber.html#method-i-transfer

{% highlight ruby linenos %}
{% include_file blog/_code/coroutine/coro-s.rb %}
{% endhighlight %}

## Lua threads (stackful, asymmetric)

Lua "threads" are stackful coroutines.  Lua has a stackless interpreter,
therefore it can easily implement stackful coroutines.  (See appendix)

Lua provides asymmetric coroutines for the sake of *simplicity* and
*portability*.

-   According to [*Coroutines in Lua*][MRI04], asymmetric coroutines behave like
    routines in the sense that the control is always transferred back to their
    callers, and is therefore easier to understand by even novice programmers.

-   Because Lua and C functions can call each other, Lua also added a limitation
    to its stackful coroutines: *a coroutine cannot yield while there are C
    function frames on its stack*.  Otherwise, Lua would require a "swap-stack"
    mechanism for C, and that will make it less portable.

In Lua, the [`coroutine.resume`] function continues the execution of a
coroutine, and [`coroutine.yield`] jumps back to the calling coroutine.

[`coroutine.resume`]: http://www.lua.org/manual/5.4/manual.html#pdf-coroutine.resume
[`coroutine.yield`]: http://www.lua.org/manual/5.4/manual.html#pdf-coroutine.yield

{% highlight lua linenos %}
{% include_file blog/_code/coroutine/coro-a.lua %}
{% endhighlight %}

The [`coroutine.wrap`] function can wrap the coroutine into an interator
function suitable for the [generic `for` statement][lua-for].

[`coroutine.wrap`]: http://www.lua.org/manual/5.4/manual.html#pdf-coroutine.wrap
[lua-for]: http://www.lua.org/manual/5.4/manual.html#3.3.5

{% highlight lua linenos %}
{% include_file blog/_code/coroutine/coro-a-wrap.lua %}
{% endhighlight %}

## Python generators (stackless, asymmetric)

Python generators are a built-in feature since Python 2.x.  They are
single-frame coroutines.

Being stackless, it is not allowed to yield from another frame than the
coroutine's frame itself.  In fact, it is syntactically impossible to do so,
because any function that contains a [`yield`][py-yieldexpr] keyword is
considered a generator function.  Calling a generator function will create a new
generator stopped at the beginning of the function, and can be resumed with the
[`next(...)`][py-next] built-in function.

To implement recursive traversal with stackless coroutines, it is common to
create one generator for each level of nested list, and yield values from
inside out, level by level.

[py-yieldexpr]: https://docs.python.org/3/reference/expressions.html#grammar-token-python-grammar-yield_expression
[py-next]: https://docs.python.org/3/library/functions.html#next

{% highlight python linenos %}
{% include_file blog/_code/coroutine/coro-gen.py %}
{% endhighlight %}

Python's [`for`][py-for] statement is a syntax sugar for calling `next(...)`
until the exception [`StopIteration`][py-stopiteratoin] is thrown.  The [`yield
from`][py-yieldstmt] statement is a syntax sugar for yielding everything from
another generator.  Using all the syntax sugar, the code above will become the
following:

[py-for]: https://docs.python.org/3/reference/compound_stmts.html#the-for-statement
[py-stopiteratoin]: https://docs.python.org/3/library/exceptions.html#StopIteration
[py-yieldstmt]: https://docs.python.org/3/reference/simple_stmts.html#the-yield-statement

{% highlight python linenos %}
{% include_file blog/_code/coroutine/coro-gen-sugar.py %}
{% endhighlight %}


## Python coroutines (WTF?)

Python 3.5 introduced the so-called "[coroutine][py-coroutine]" function, using
the syntax `async def foo(...)`.  Like generator, calling a "coroutine function"
creates a coroutine object.  Inside a coroutine function, it can `yield`, but
not `yield from`.

Python coroutines support a new [`await`][py-await] expression, but its
semantics is [very vaguely defined][py-await] as "suspend the execution of
coroutine on an awaitable object", whatever "on an awaitable object" means. That
is in stark contrast to [C++20's highly detailed semantics][c++20-coawait] of
the `co_await` expression.

>   In the face of ambiguity, refuse the temptation to guess.
>
>   *-- [Zen of Python]*

From Python's official language reference and [PEP 492][py-pep492], it looks
like the new async/await mechanism is intended for asynchronous programming (see
appendex), a very specific scenario where multiple "coroutines" are scheduled by
a kind of "event loop".  This violates the characterestic of coroutine that
coroutine switching is part of the control flow.  This makes Python "coroutines"
more similar to a variant of "goroutines" that leaks the abstraction of
task-switching.

And the user almost always use async/await with the [`asyncio`][py-asyncio]
module.  I don't think it is good to introduce a **language feature** that is
only useful to one module.

Because it is so confusing, I am not going to do the task using Python
"coroutines".

[py-coroutine]: https://docs.python.org/3/reference/compound_stmts.html#coroutines
[py-await]: https://docs.python.org/3/reference/expressions.html#await
[py-pep492]: https://peps.python.org/pep-0492/
[py-asyncio]: https://docs.python.org/3/library/asyncio.html
[c++20-coawait]: https://timsong-cpp.github.io/cppwp/n4861/expr.await#5
[Zen of Python]: https://legacy.python.org/dev/peps/pep-0020/


## Python greenlet (stackful, symmetric)

### Symmetric

The third-party library [greenlet] provides stackful symmetric coroutines for
CPython and PyPy.

Greenlets are stackful.  According to the [documentation][greenlet-concept], a
"greenlet" is "*a small independent pseudo-thread*", something that can be
"*thought about as a small stack of frames; the outermost (bottom) frame is the
initial function you called, and the innermost frame is the one in which the
greenlet is currently paused*".

Greenlets are symmetric.  One greenlet can switch to another using the
[`glet.switch()`][greenlet-switch] method to pass a value, or
[`glet.throw()`][greenlet-throw] to switch and immeidately raise an exception.

Implementation-wise, the official greenlet uses platform-specific assembly code
(for [amd64][greenlet-asm-amd64], [aarch64][greenlet-asm-aarch64],
[riscv][greenlet-asm-riscv], etc.) to switch native stacks, similar to what
[Boost Context] does.

PyPy [implements the greenlet API][pypy-greenlet] using
[stacklets][pypy-stacklet], which is PyPy's own swap-stack mechanism.  Like
[Boost Context] and [greenlet], PyPy also uses platform-specific assembly code
to switch between stacks.  (See the code for [x86-64][pypy-stacklet-x86-64],
[aarch64][pypy-stacklet-aarch64], [mips64][pypy-stacklet-mips64], etc.  Sorry,
RISC-V.)

[greenlet]: https://greenlet.readthedocs.io/en/latest/index.html
[greenlet-concept]: https://greenlet.readthedocs.io/en/latest/greenlet.html
[greenlet-switch]: https://greenlet.readthedocs.io/en/latest/api.html#greenlet.greenlet.switch
[greenlet-throw]: https://greenlet.readthedocs.io/en/latest/api.html#greenlet.greenlet.throw
[greenlet-asm-amd64]: https://github.com/python-greenlet/greenlet/blob/master/src/greenlet/platform/switch_amd64_unix.h
[greenlet-asm-aarch64]: https://github.com/python-greenlet/greenlet/blob/master/src/greenlet/platform/switch_aarch64_gcc.h
[greenlet-asm-riscv]: https://github.com/python-greenlet/greenlet/blob/master/src/greenlet/platform/switch_riscv_unix.h
[pypy-greenlet]: https://doc.pypy.org/en/latest/stackless.html#greenlets
[pypy-stacklet]: https://doc.pypy.org/en/latest/stackless.html#stacklets
[pypy-stacklet-x86-64]: https://foss.heptapod.net/pypy/pypy/-/blob/branch/default/rpython/translator/c/src/stacklet/switch_x86_64_gcc.h
[pypy-stacklet-aarch64]: https://foss.heptapod.net/pypy/pypy/-/blob/branch/default/rpython/translator/c/src/stacklet/switch_aarch64_gcc.h
[pypy-stacklet-mips64]: https://foss.heptapod.net/pypy/pypy/-/blob/branch/default/rpython/translator/c/src/stacklet/switch_mips64_gcc.h

{% highlight python linenos %}
{% include_file blog/_code/coroutine/coro-greenlet.py %}
{% endhighlight %}


### Emulate asymmetric coroutine using the `parent` field

We have just demonstrated that greenlets are symmetric.  However, each greenlet
has a [parent][greenlet-parent].  It is the coroutine to switch to when the
current coroutine terminates, normally or by exception.  However, it doesn't
mean greenlets are asymmetric because the parent can be changed at any time
during execution, and it is not wrong to explicitly `switch` to the parent.

We can rewrite our last example and use the `glet.parent` field instead of our
own `parent` variable to record the parent.

[greenlet-parent]: https://greenlet.readthedocs.io/en/latest/greenlet.html#greenlet-parents

{% highlight python linenos %}
{% include_file blog/_code/coroutine/coro-greenlet-a.py %}
{% endhighlight %}


## Rust async/await (stackless, asymmetric, not designed for coroutine)

Rust's `async` and `await` keywords are designed for asynchronous programming
(see appendix).  There is [a dedicated book][rust-async-book] that covers
asynchronous programming in Rust.

Like Python generator functions, an [`async` function][rust-async-func] or an
[`async` block][rust-async-block], when executed, do not execute their bodies
immediately.  Instead, they create a `Future` object that holds the execution
context of that function or block.  The `Future::poll` method will let it run
until it yields (on an `await` site) or finishes.

The [`await` expression][rust-await] can only be used in one `async` thing to
wait for another `async` thing to finish.  It "polls" the other `async` and, if
the other `async` finished, it continues;  otherwise, it yields from current
`async`, and the current `async` can be resumed later.  From this perspective,
an `async` thing is a coroutine (underneath), and `await` is a conditional
yield.

Implementation-wise, [the documentation suggests][rust-async-state-machine] that
Rust decomposes an `async` function (or block) into a state machine where each
state represents an `await` site.  Each time it is "polled", it runs to the next
*pending* await site.

**Async/await is not supposed to be used like coroutines** as we discussed in
this post.  In fact, the book [Asynchronous Programming in
Rust][rust-async-book] contrasts async/await against coroutines and [claims it
has advantages over coroutines][rust-async-vs-coroutine].

However, it is possible to abuse the async/await mechanism to exploit its
underlying coroutine. We need to call `Future::poll` directly, which is seldom
done in practice unless we are implementing the "executor".  We will invoke an
`async` function recursively to traverse our nested list.  Every time we visit a
number, we write the number in a shared variable (note that "yielding" in an
async/await framework is not for producing partial results to the invoker), and
`await` until the consumer writes another shared variable to indicate "it has
read the value".  This `ResultReporter` struct effectively behaves like a
zero-capacity channel.  The sender can only continue after the receiver takes
the value.

[rust-async-book]: https://rust-lang.github.io/async-book/
[rust-async-func]: https://doc.rust-lang.org/reference/items/functions.html#async-functions
[rust-async-block]: https://doc.rust-lang.org/reference/expressions/block-expr.html#async-blocks
[rust-async-statemachine]: https://rust-lang.github.io/async-book/01_getting_started/04_async_await_primer.html
[rust-async-vs-coroutine]: https://rust-lang.github.io/async-book/01_getting_started/02_why_async.html#async-vs-other-concurrency-models
[rust-await]: https://doc.rust-lang.org/reference/expressions/await-expr.html

{% highlight rust linenos %}
{% include_file blog/_code/coroutine/coro-async.rs %}
{% endhighlight %}


## Rust coroutine crate (stackful, asymmetric)

The third-part crate [`coroutine`][rust-coroutine] provides stackful asymmetric
coroutines.  It is built upon the `context` crate (see below).

It looks like this crate has not been maintained for quite some time.  It
depends on a deprecated feature `FnBox` which no longer exists in the current
toolchains.  I'll not do the task using the `coroutine` crate.

[rust-coroutine]: https://docs.rs/coroutine/latest/coroutine/


## Rust context crate (stackful, symmetric)

The third-part crate [`context`][rust-context] is similar to [Boost Context]. It
provides the abstraction of stackful symmetric coroutines.

It implements swap-stack using machine-specific assembly code
([x86-64][rust-context-x86-64], [AArch64][rust-context-arm64],
[ppc64][rust-context-ppc64], sorry RISC-V).

The [`Context::resume`][rust-context-resume] method switches the thread to the
other coroutine, and pass the context of the original coroutine so the thread
can switch back.

[rust-context]: https://docs.rs/context/latest/context/index.html
[rust-context-x86-64]: https://github.com/zonyitoo/context-rs/blob/master/src/asm/jump_x86_64_sysv_elf_gas.S
[rust-context-arm64]: https://github.com/zonyitoo/context-rs/blob/master/src/asm/make_arm64_aapcs_elf_gas.S
[rust-context-ppc64]: https://github.com/zonyitoo/context-rs/blob/master/src/asm/jump_ppc64_sysv_elf_gas.S
[rust-context-resume]: https://docs.rs/context/latest/context/context/struct.Context.html#method.resume

{% highlight rust linenos %}
{% include_file blog/_code/coroutine/coro-context.rs %}
{% endhighlight %}

# Appendices

## Why this task?

The purpose of this task is to compare *symmetric*, *asymmetric*, *stackful* and
*stackless* coroutines.

This task is natual to implement with coroutines.  It is a "generator",
something that gives out a sequence of values as it executes.  It is natural to
have one coroutine that gives out the values, and another coroutine to consume
the values, and the two can run in alternation.

This task is also much easier with stackful coroutines than stackless
coroutines.  It is easier to traverse a recursive data structure using
recursoin, and recursion needs a stack.  Stackful coroutines can handle the
stack quite trivially, but when using stackless coroutines, we have to do some
hack and chain up multiple coroutines to form a stack of coroutines, and yield
values through multiple layers of coroutines.

## What are coroutines anyway?

According to [Conway][Conway1963], a **coroutine** is "*an autonomous program
which communicates with adjacent modules as if they were input or output
subroutines*".  Coroutines are subroutines all at the same level, each acting as
if it were the master program when in fact there is no master program.

Coroutines has the following characterestics. (See [*Coroutines in
Lua*][MRI04])

1.  The values of data local to a coroutine persist between successive calls.
2.  The execution of a coroutine is suspended as control leaves it, only to
    carry on where it left off when control re-enters the coroutine at some
    later stage.

The first characterestic means coroutines can be resumed from where it paused.

In the second characterestic, "as control leaves it" means it is the programmer
that decides *where* to pause a coroutine, not the implicit scheduler.  Control
flow is part of a program, not the runtime.

For this reason, the "fibers" in Ruby, the "generators" in Python, the
"threads" in Lua, and the "user contexts" in the `swapcontext` POSIX API are all
coroutines, despite not being called "coroutines".  The programmer explicitly
transfers control from one to another.

On the other hand, a "goroutine" in the Go programming language is not a
coroutine, despite the similar name.  In fact, [the official document][go-stmt]
defines a "goroutine" as "an independent concurrent thread of control", i.e. a
thread.

<small>Note: Goroutine is a threading mechanism implemented in the user space,
an "M\*N" green thread system. The Go runtime schedules M goroutines on N native
threads. It is the scheduler that switches a native thread between different
goroutines at unspecified time and locations, usually when IO operations may
block, or when queues are full or empty.  To the programmer, a goroutine is just
like a thread: it keeps going forward.  It may block, but not because the
programmer asked it to yield, but because some requests cannot be satisfied,
such as queues and IO. </small>

Coroutines are not mutually exclusive with threads.  Each thread is executing
one coroutine at a time, and each thread may jump from one coroutine to another
according to the control flow in the program.

[Conway1963]: https://dl.acm.org/doi/10.1145/366663.366704
[MRI04]: http://www.lua.org/doc/jucs04.pdf
[go-stmt]: https://golang.google.cn/ref/spec#Go_statements

## Symmetric and asymmetric coroutines.

With *symmetric* coroutines, all coroutines are equal.  A thread can jump from
any coroutine to any other coroutine.  When switching, the programmer always
needs to specify which coroutine to jump to, i.e. destination.

With *asymmetric* coroutines, coroutines have a parent-child relation.  There
are two different operations that jumps between coroutines, namely `resume` and
`yield`.  When a thread "resumes" a coroutine, the destination becomes the child
of the source coroutine, until it "yields", when the thread jumps back from the
child to the parent coroutine.  A coroutine cannot be resumed when it already
has a parent.  When yielding, the programmer doesn't need to specify the
destination, because the destination is always implicitly the parent coroutine.

Symmetric and asymmetric coroutines have equal expressive power, and they can
implement each other.  See [Coroutines in Lua][MRI04] for more details.

## Stackful and stackless coroutines.

A *stackful* coroutine has its own execution stack.  A thread can switch
coroutine when the current coroutine has any number of frames on its stack. When
switching, the thread saves the entire stack and switches to a whole new stack
(implementation-wise, it only needs to save registers, change the stack pointer,
and restore registers from the new stack).  

A *stackless* coroutine has only one frame.  Therefore, a coroutine is usually
defined by a "coroutine function". From my knowledge, all stackless coroutines
are asymmetric.  A coroutine function can only yield within the coroutine
function itself.  That means when a coroutine C1 calls another function F2, the
thread cannot yield from C1 while executing F2;  when a coroutine C1 resumes
another coroutine C2, the thread can only yield from C2 back to C1, but not
directly from C2 to C1's parent.

Stackful coroutines are more powerful, but need some kind of "swap-stack"
mechanism (see below) to implement.  Stackless coroutines are more restricted,
but does not require swap-stack.

## The swap-stack mechanism

Conceptually, the context of nested function calls is a stack, a
last-in-first-out data structure, called the *control stack*, often simply
called a *stack*.

A control stack has many *frames*.  Each frame contains the execution context of
a function activation, (and this is why a frame is also known as an *activation
record*).  The context includes the program counter (PC) as well as the values
of local variables.  The top frame is the context of the most recently entered
function, and is the only frame on a stack that is active.  All other frames are
paused at a call site, waiting for the called function (the frame above it) to
return.

In most programming languages, a thread is always bound to one stack. But more
generally, a thread can switch among different stacks.  When a thread switches
from one stack to another, it pauses the top frame as well making the whole
stack paused.  Then it switches its stack pointer to the new stack, and resume
the top frame of the new stack, therefore continue from where that frame was
paused.

This is basically what *stackful coroutine* does.  Each coroutine has a stack,
which can be paused and resumed.  When resumed, it continues from where it was
paused.


### Implementing swap-stack with compiler

Implementation-wise, if the language is compiled, it needs a special instruction
sequence that does the following things:

1.  Save live registers on the top of the current stack, and
2.  set the stack pointer (SP) register to the destination stack, and
3.  restore the saved registers from the top of the destination stack.

The C and C++ programming languages themselves do not have support for
swap-stack.  In practice, we usually rely on libraries or compiler extensions to
do that in C or C++.

Here I give two examples of implementations of swap-stack for compiled code.

1.  One is from [Boost Context].  It is implemented as a library in the assembly
    language, therefore it has to be platform-specific. Here are the code for
    [x86-64][boostctx-x64], [ARM64][boostctx-arm64] and
    [RISCV64][boostctx-riscv64].  Because it is implemented as a library, it can
    only depend on the application binary interface (ABI) of the platform.  In
    this case, the assembly code has to conservatively save all callee-saved
    registers no matter whether they are still in use or not, because as a
    library, it does not have the liveness information the compiler has.

2.  The other is an LLVM extension created by [Dolan et al.][DMG13].  As part of
    a compiler framework, it can identify and save only live registers, making
    it much more efficient than library-based approaches.

[Boost Context]: https://www.boost.org/doc/libs/1_80_0/libs/context/doc/html/index.html
[boostctx-x64]: https://github.com/boostorg/context/blob/develop/src/asm/jump_x86_64_sysv_elf_gas.S
[boostctx-arm64]: https://github.com/boostorg/context/blob/develop/src/asm/jump_arm64_aapcs_elf_gas.S
[boostctx-riscv64]: https://github.com/boostorg/context/blob/develop/src/asm/jump_riscv64_sysv_elf_gas.S
[DMG13]: http://dx.doi.org/10.1145/2400682.2400695


### Implementing swap-stack with interpreter

If the language is interpreted, it depends.  An interpreter can be stackful or
stackless, and even stackless interpreters can allow the interpreted functions
to call foreign C functions.

A stackful interpreter uses the native (C) stack to implement function
invocation.  Such interpreters usually has the following form:

```c
void interpret_function(Frame *frame) {
    while (true) {
        switch (current_instruction().type()) {
        case ...:
            ...
        case CALL:
            ...
            Frame *new_frame = create_frame(called_function);
            interpret_function(new_frame);  // Recursive call
            ...
        }
    }
}
```

Because a language-level function call corresponds to an interpreter-level
(C-level) function call, each frame in the interpreted language corresponds to a
C frame.  It is difficult to implement swap-stack with stackful interpreter
because it needs to swap out the C stack (which has C frames) in order to swap
out the interpreted language stack.  As we have discussed before, C does not
have native support for swap-stack, and it needs libraries written in assembly
language or compiler extensions to do so.

On the other hand, a stackless interpreter does not turn language-level function
calls into C-level function calls.  A stackless interpreter usually has the
following form:

```c
void interpret_function(Frame *initial_frame) {
    Frame *current_frame = initial_frame;
    while (true) {
        switch (current_instruction().type()) {
        case ...:
            ...
        case CALL:
            ...
            Frame *new_frame = create_frame(called_function);
            new_frame->parent = current_frame;
            current_frame = new_frame;  // Only replace the frame pointer
            ...
        }
    }
}
```

A stackless interpreter always remains in the single `interpret_function`
function activation even when the interpreted language program makes a call.
Swap-stack is relatively easier to implement with stackless interpreter,
because it does not need ot swap out any C frames...

... unless it allows foreign function calls.  If the stackless interpreter
allows the interpretd language to call foreign C functions, then C functions
must have frames on some stack.  Then we face the same problem as implementing
swap-stack for compiled languages.

### The Mu micro virtual machine

I designed the [Mu micro virtual machine][mu], and it is the main part of [my
PhD thesis].  Swap-stack is a very important mechanism of the Mu micro VM, and
it is designed to be supported by the JIT compiler.  It enables the
implementation of symmetric stackful coroutines, and it is the foundation of
other VM mechanisms, such as trapping and [on-stack replacement
(OSR)][osr-paper].  If you are interested, read [Section
5.3.6][phd-thesis-swapstack] of [my thesis][phd-thesis].

[mu]: https://microvm.github.io/
[phd-thesis]: https://wks.github.io/downloads/pdf/wang-thesis-2018.pdf
[phd-thesis-swapstack]: https://wks.github.io/downloads/pdf/wang-thesis-2018.pdf#subsection.5.3.6
[osr-paper]: https://wks.github.io/downloads/pdf/osr-vee-2018.pdf

<!--
vim: tw=80
-->
