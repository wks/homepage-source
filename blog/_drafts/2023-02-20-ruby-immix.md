---
layout: post
title: "Immix for Ruby"
tags: mmtk ruby
---

I have been working on [MMTk support for CRuby][mmtk-ruby] (the official Ruby
interpreter, written in C, as opposite to JRuby, TruffleRuby, etc.).  I managed
to get CRuby working with the Immix garbage collector via MMTk.  It is the first
time CRuby can run with an evacuating copying collector.  Isn't that exciting?
In this article, I'll introduce my experience of getting MMTk and Immix working,
and the challenges I encountered. I am surprised by how many assumptions CRuby
developers have made about its garbage collector throughout the VM
implementation, and how bad they are for supporting new GC algorithms.

[mmtk-ruby]: https://github.com/mmtk/mmtk-ruby

# Ruby, a language humans

> Often people, especially computer engineers, focus on the machines. They
> think, "By doing this, the machine will run fast. By doing this, the machine
> will run more effectively. By doing this, the machine will something something
> something." They are focusing on machines. But in fact we need to focus on
> humans, on how humans care about doing programming or operating the
> application of the machines. We are the masters. They are the slaves. 
> 
> *-- Matsumoto Yukihiro on [the philosophy of Ruby][ruby-interview].*

[ruby-interview]: https://www.artima.com/articles/the-philosophy-of-ruby#part4

Ruby, a language for humans, has attracted millions of programmers around the
world with its elegant syntax and extensibility.  If you want a dynamic web
site, you can prototype it in an hour using the Ruby on Rails framework.  If you
only want a little script to count words in a text file, its built-in regular
expression support makes it handy.

If you want to use your favourite C library in Ruby, that's OK, too. You can
implement your Ruby module in C!  Just (1) define a Ruby class, (2) define an
`rb_data_type_t`, and (3) define a C struct.  Then you call
`TypedData_Make_Struct` and...  Voila! Your shiny new Ruby object backed by a C
struct!

<small>*(Note: I intentionally omitted the [`ffi`][ffi] module.  Personally, I
prefer `ffi` over C extensions, but `ffi` is not related to the GC or object
scanning problems I'll discuss in this article.)*</small> 

[ffi]: https://github.com/ffi/ffi

You may even want to race [your old pure Ruby module][liquid] against [your C
version][liquid-c], and the C version [probably wins][liquid-c-performance].

Oh, don't forget to teach Ruby how to do garbage collection for your C type
using the `rb_data_type_t` structure you just defined.  Teach the GC (1) how to
mark its `VALUE` fields, (2) how to forward the `VALUE` fields when the GC moves
object, and (3) how to...

[liquid]: https://github.com/Shopify/liquid
[liquid-c]: https://github.com/Shopify/liquid-c
[liquid-c-performance]: https://github.com/Shopify/liquid-c#performance

Wait!  What is garbage collection?  What is marking?  Why does the GC move
objects?

And why would I care about those VM implementation details?  We are humans, not
machines, aren't we?

# No peaceful world

> 哪有什么岁月静好，不过是有人替你负重前行。
> (There is no such thing as a peaceful world. We feel like so because someone
> take on the burdens.)
>
> *--苏心 (Xin Su)*

Ruby programmers don't need to worry about freeing their objects because the
garbage collector takes care of reclaiming unused objects.

If you instantiate classes written in Ruby, the CRuby runtime knows where
instance variables are held, and the GC can traverse from the object to its
children.

<small>*(I bet you don't know that CRuby sometimes stores instance variables in
a global hash table outside the object.  It's called a "GLOBAL Instance Variable
TaBLe", or `global_ivtbl` for short.)*</small>

If you instantiate *built-in* types implemented in C, CRuby still gets you
covered. CRuby developers have taught the GC how to scan those special objects
by writing functions that [mark their children][gc_mark_children] and [update
their reference fields][gc_update_object_references].

[gc_mark_children]: https://github.com/ruby/ruby/blob/693e4dec236e14432df97010082917a3a48745cb/gc.c#L7242
[gc_update_object_references]: https://github.com/ruby/ruby/blob/693e4dec236e14432df97010082917a3a48745cb/gc.c#L10550

But if you define you own Ruby type backed by your own C struct, CRuby doesn't
know its layout.  You have to teach CRuby how to scan them.

How?

You write a *marking function* that calls `rb_gc_mark` on the value of each
`VALUE` fields.

```c
struct Foo {
    VALUE x;
    int y;
    VALUE z;
}

void foo_mark(struct Foo *obj) {
    rb_gc_mark(obj->x);
    rb_gc_mark(obj->z);
}

const rb_data_type_t foo_data_type = {
    "Foo",
    { foo_mark, foo_free, foo_memsize }, // Let's ignore foo_free and foo_memsize for now
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};
```

That's it!  Well, at least that's how we write C extensions 10 years ago.
However, since CRuby introduced compacting GC, you should replace `rb_gc_mark`
with `rb_gc_mark_movable`, and add an *updating function* that reassigns each
`VALUE` field with the return value of `rb_gc_location`.

```c
struct Foo {
    VALUE x;
    int y;
    VALUE z;
}

void foo_mark(struct Foo *obj) {
    rb_gc_mark_movable(obj->x);
    rb_gc_mark_movable(obj->z);
}

void foo_update(struct Foo *obj) {
    obj->x = rb_gc_location(obj->x);
    obj->z = rb_gc_location(obj->z);
}

const rb_data_type_t foo_data_type = {
    "Foo",
    { foo_mark, foo_free, foo_memsize, foo_update },
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};
```

That's confusing.  What does `rb_gc_mark` do?  What's the difference between
`rb_gc_mark` and `rb_gc_mark_movable`?  And why do we need an update function
too?  I have heard Python programmers doing "INC" and "DEC" operations all the
time.  Why don't we need them in Ruby?

To understand these issues, we need to understand the differences between GC
algorithms.


# Sweeping, compacting and evacuating garbage collectors

Garbage collection (GC) is a form of automatic memory management that reclaims
objects when they are no longer reachable.  There are many garbage collection
algorithms out there.  Reference counting (RC), mark-sweep, mark-compact,
semispace, Immix, generational mark-compact, generational Immix, RC-Immix,
sticky Immix, pauseless, C4, garbage-first (G1), Shenandoah, ZGC, Sapphire, LXR,
to name a few.  Each GC algorithm has a way to allocate object, identify garbage
and reclaim memory.

algorithm    | allocation                                 | identification      | reclamation
-------------|--------------------------------------------|---------------------|--------------------
naive RC     | free-list *(slow)*                         | reference counting  | sweeping to free-list *(fast)*
mark-sweep   | free-list *(slow)*                         | tracing             | sweeping to free-list *(fast)*
mark-compact | bump-pointer *(fast)*                      | tracing             | compaction *(very slow)*
semispace    | bump-pointer *(fast)*                      | tracing             | evacuation *(slow)*
Immix        | objects: bump-pointer<br>blocks: free-list | tracing             | normal: sweeping lines/blocks<br>defrag: evacuation

Let's compare mark-sweep, mark-compact and semispace, and ignore naive RC and
Immix for now.  All of them use tracing to identify garbage, and their
difference lies in their allocators.

-   Mark-sweep uses a *free-list allocator*.  Free-list allocation is slow, but
    dead objects can be simply added back to the free-list, which is fast.  It
    never moves any object, which saves time, but can cause heap fragmentation.
-   Mark-compact and semispace use a *bump-pointer allocator*.  Bump allocation
    is fast, but it can only allocate into a contiguous region of free space.
    Therefore, those collectors need to move objects to defragment the heap,
    which is slow.
    -   Mark-compact does this by *compacting*, that is, moving all live objects
        to one side of the heap so that the rest of the heap is a contiguous
        region of free space.  This is extremely slow because it needs to avoid
        overwriting other live objects while moving objects.
    -   Semispace does this by *evacuating* all live objects to the other half
        of the heap memory, and freeing the entire old half at once.  This is
        not as slow as compacting, but Semispace can only utilise 50% of the
        total heap space.

Mark-sweep, mark-compact and semispace represent the three basic ways of
reclaiming used memory, i.e. *sweeping to free-list*, *compacting* and
*evacuating*.

## Ruby's GC

What GC algorithm does Ruby use?

Long time ago, Ruby had a non-moving mark-sweep garbage collector.  Mark-sweep
is simple.  It traverse the object graph, starting from roots and following
reference fields.  Objects that are not transitively reachable from roots are
dead. The GC then goes through all allocated cells to find dead objects, and
reclaim their cells.

Later, people probably found that heap fragmentation was a problem.  CRuby
developers then introduced compaction, making it a hybrid mark-sweep-compact GC.
Unlike pure mark-compact algorithms, objects are still allocated from
size-segregated free-lists.  It still does mark-sweep GC most of the time.  But
when the user calls `GC.compact`, or if auto-compaction is enabled, the GC will

1.  traverse the heap once to mark live object, and
2.  go through all cells with live objects and move them to free pages, and
3.  go through all cells with live objects again and update their reference
    fields.
    -   Note: After objects have been moved, pointers to the moved objects
        need to point to their new addresses.

Then large chunks of memory can be returned to the operating system.

<small>*I guess the nature of this GC algorithm explains why auto-compaction is
disabled by default, because it allocates objects as slowly as mark-sweep, and
compacts the heap as extremely slowly as mark-compact.*</small>

## GC and Ruby's C extension API

You see.  The GC algorithm CRuby uses dictates the design of Ruby's C extension
API:

-   When CRuby used mark-sweep, *marking functions* were sufficient.
    -   Inside marking functions, the idiomatic `rb_gc_mark(obj->x);` statement
        gives the GC the field value without updating the field.
-   After compaction was introduced, *updating functions* were needed, too.
    -   Inside updating functions, the `rb_gc_location` function gets the new
        location of an object, and the idiomatic statement `obj->x =
        rb_gc_location(obj->x);` updates the field by assigning the return value
        back to the field.

The "INC" and "DEC" operations are not needed for CRuby because it doesn't use
reference counting.

<small>*(Actually, what the GC needs is just a way to **visit** all the
reference fields of any given object.  Instead of having separate marking and
updating functions, a more clever API design would be letting
the C extension pass the **offsets** or **addresses** of the reference fields to
the GC so that it can read or update those fields, regardless of whether it is
using mark-sweep, mark-compact or any other GC algorithms.  We will come to this
topic later, but the marking functions and updating functions work well for the
current CRuby GC.)*<small>

### Legacy gems

The bad news is, even today, there are still lots of third-part Ruby libraries
which were developed before Ruby had compacting GC, and still do not have
updating functions. CRuby developers decided to maintain compatibility with
those legacy libraries.

-   The semantics of the publicly exposed `rb_gc_mark` function was changed so
    that it not only marks the object, but also **pins** it.
-   A new function `rb_gc_mark_movable` was introduced.  It marks the object,
    but does not pin it.

Then during compaction, objects marked with `rb_gc_mark` will stay in place,
while other objects can be freely moved. By doing so, legacy C extensions can
keep working without updating fields because children objects are guaranteed not
to move.  Newer C extensions can enjoy the benefit of defragmentation by marking
the children movable.  There is some performance penalty because some objects
cannot be moved.  Despite of that, it works.


# MMTk

For those who don't know yet, [MMTk] was part of JikesRVM and is now a
VM-independent Rust library.  It is a framework for garbage collection.  It
contains abstractions of spaces, allocators, tracing, metadata, etc.  It also
has a powerful work packet scheduler for multi-threaded GC.  Its VM binding API
makes the boundary between MMTk and the VM clear and efficient.  Currently, MMTk
includes several canonical GC algorithms (we call them "plans" in MMTk), such as
NoGC, MarkSweep, MarkCompact, Semispace, GenCopy, as well as productional ones
such as Immix and GenImmix.

<small>*(Yes. NoGC is a GC algorithm that never reclaims any memory and crashes
when memory is exhausted.  It's known as "epsilon GC" in OpenJDK.)*</small>

[MMTk]: https://www.mmtk.io/

I have been working on [mmtk-ruby], the VM binding for CRuby, so that CRuby can
use MMTk as its garbage collector.  Following [Angus Atkinson][angus-homepage]'s
[initial work][angus-lca2022] that gets CRuby running with NoGC, I have hacked
CRuby enough to make it working with MarkSweep last year, and Immix recently.
(Don't get too excited.  It still crashes here and there.)  

[angus-homepage]: https://angusat.kinson.it/
[angus-lca2022]: https://kinson.it/talks/lca2022

## Supporting MMTk for any other VM, step by step

Usually, adding MMTk support for a given VM takes several steps.

1.  Support NoGC.
2.  Support MarkSweep.
3.  Support Semispace.
4.  Support GenCopy.
5.  Implement allocation and write barrier fast path using the JIT compiler.

The first step is, maybe surprising to some people, supporting NoGC.  In this
step, we disable the VM's existing GC, and hijack the VM's object allocation
mechanism so that it allocates using MMTk.  Although NoGC is not a realistic GC
algorithm, this step helps us identify the boundary between the VM and the GC.
And it is the easiest.  The VM just calls the `mmtk::memory_manager::alloc`
function, and it should just work... well... until the memory is exhausted.
Despite of this, Angus managed to run a Rails web server with NoGC.

The next step is supporting MarkSweep.  MarkSweep is a non-moving collector, so
we only need to focus on identifying garbage and not worry about pointer
updating at this moment.  The VM needs to implement a stop-the-world mechanism
to stop all mutator threads when GC is triggered, a root scanner to visit roots
on the stack as well as in global data structures, and an object scanner that
identifies reference fields in any given object.

The third step is supporting Semispace.  Semispace is a copying collector, and
it copies every single live object during GC.  This step forces us to get object
copying and reference forwarding right.  The VM needs to implement a function to
copy a given object.  If the VM doesn't assume the GC never moves object, this
step should be trivial; but if not, this step could be painful.

The fourth step is supporting GenCopy, the generational copying collector with a
Semispace mature space.  This collector requires write barrier.  MMTk provides
the write barrier implementation, exposed as
`mmtk::memory_manager::object_reference_write`, and the VM needs to ensure it is
called whenever writing a reference field.

With stop-the-world, root scanning, object scanning, object copying and write
barriers handled, the VM should be able to use other GC algorithms MMTk
provides, too, such as Immix and GenImmix.  

The fifth step is supporting fast paths.  This is an optimisation.  For
performance reasons, the VM should inline the commonly executed fast paths of
object allocation and write barriers... well... if it care about performance.

# MMTk and CRuby

Great!.  Let's use MMTk for CRuby, and make all of its GC algorithms available
for CRuby!

Unfortunately, there is a catch.  CRuby needs object pinning, and that limits
what GC algorithms we can use.

## CRuby and object pinning

Why does CRuby pin objects?

### Conservative stack scanning

CRuby is, as the name suggests, written in C.  Methods of built-in types and
types in C extensions can be implemented in C.  CRuby allows C local variables
to hold references to Ruby objects using the `VALUE` type.  A `VALUE` can be a
direct pointer to a heap object, without using any indirection table (like
OpenJDK and Lua) or reference counting (like CPython).  Here comes the problem:
C compilers cannot generate stack maps (the metadata that records "which offsets
of a frame contain references").  How can the GC find object references on the
stack?

CRuby scans stacks conservatively.  It assumes that every word on the stack can
*potentially* be a reference.  If a word *happens to* be the address to an
object, the GC will consider it as an object reference, and keep that object
alive.  However, objects kept alive this way cannot be moved.  If the object is
moved, the reference on the stack needs to be updated.  But that may be
incorrect.  What if the word is just an integer that happens to have the same
value as an object address?  For this reason, the object must be pinned.

The `rb_gc_mark_machine_stack` function calls `gc_mark_maybe` on each word in
the machine stack, and `gc_mark_maybe` does some filtering and calls
`gc_mark_and_pin`.

### Legacy C extensions with un-update-able fields

Like I mentioned in previous sections, some C extensions were developed before
Ruby had copying GC. They cannot update their object fields during GC.  To keep
those C extensions working, CRuby pins the objects *pointed by* those
un-update-able fields during the marking phase, and refuse to move them during
the sweeping phase.

This is why the legacy `rb_gc_mark` function is implemented with
`gc_mark_and_pin`.

### Legacy and conservative built-in objects with un-update-able fields

CRuby has some built-in object types that have un-update-able fields.  They
include:

-   Any object that uses "global instance variable table", or `global_ivtbl`.
-   `Hash` that has the `compare_by_identity` function called.
-   Some `T_IMEMO` types, including
    -   `imemo_ifunc`
    -   `imemo_memo`
    -   `imemo_iseq`
    -   `imemo_ast`
    -   `imemo_tmpbuf`
    -   `imemo_parser_strstream`
-   Some `T_DATA` types, such as `VM/Thread`.

Some of them are really conservative.  For example, `imemo_tmpbuf` is used to
implement `ALLOCV`.  The GC scans every word in `imemo_tmpbuf` conservatively,
like scanning words on the stack.  This makes `ALLOCV` behave just like
`alloca`: allocating temporary memory that is automatically recycled after the
current function returns, except that `ALLOCV` allocates the buffer on the heap
(when size is large).  Users use `ALLOCV` like a safer variant of `alloca`.

`Hash` needs to pin its children when comparing by identity because CRuby uses
the object address as the hash key of such hash tables.  If an object is moved,
the hash code will change.  Therefore, the key has to be pinned.

<small>*(NOTE: a wiser way to hash objects by identity is using **address-based
hashing**.  It uses the object address as hash code, but when the object is
moved, the GC saves the old address in a hidden field of the new object, and use
the saved address as its "identity hash".  This preserves the "identity hash" of
an object across copying GC.)*</small>

Other objects pin their children solely for historical reasons.  Examples
include `imemo_iseq` and the `global_ivtbl`.  You may assume Ruby developers
always keep built-in types up to date and make use of `rb_gc_mark_movable` and
`rb_gc_location`, but it doesn't seem to be this case.  If there are not many
instances of such objects, developers will tend to leave the code as is because
the performance impact of not being able to move objects is small (at least it
is small with the current GC which doesn't move objects by default).

## Supporting MMTk for CRuby, step by step

Because of object pinning, we have to deviate a little bit from the usual steps
of supporting a VM in order to support CRuby.

1.  Supporting NoGC.
2.  Supporting MarkSweep.
3.  Supporting Immix.  Semispace doesn't support object pinning, while Immix
    does.
4.  Supporting StickyImmix.  Similarly, neither GenCopy nor GenImmix support
    object pinning, while Sticky Immix is variant of GenImmix that supports
    object pinning.
5.  Implementing fast paths in YJIT.

Angus had done step 1 before, and I have just finished step 3 recently (although
bugs still exist).

## The Immix GC algorithm

*(For more information about Immix, read [this paper][BM08].)*

[BM08]: https://users.cecs.anu.edu.au/~steveb/pubs/papers/immix-pldi-2008.pdf

![Immix Heap Organization]({% link assets/img/immix-heap-organization.png %})

The Immix collector organises the heap into equally sized blocks (usually 32KB),
and each block has many lines (usually 128 bytes).  An object may span multiple
lines, but cannot cross block boundaries.

<small>*(Note: Objects larger than 8KB are allocated in a dedicated "large
object space".  It always allocates multiples of page-sized memory, and it uses
the mark-sweep GC algorithm.  You really don't want to move large objects
because that's horribly slow.)*</small>

When allocating, it uses a bump-pointer allocator to allocate objects within a
page, skipping occupied lines if needed.  During GC, when traversing the heap,
lines occupied by live objects are marked; when sweeping, unmarked lines are
reclaimed.  From time to time, Immix evacuates a small number of blocks that are
heavily fragmented (having many unallocated "holes" between occupied lines).  We
see:

-   The Immix allocator allocates objects as fast as mark-compact and semispace.
-   Normal (non-defragmenting) GCs are as fast as mark-sweep.
-   It is capable of defragmenting the heap by evacuating, but doesn't have to
    move objects in every GC, and doesn't need to move every single object (or
    even the majority of all objects).

These characteristics make Immix an overall efficient GC algorithm, balancing
allocation speed, GC speed and heap utilisation.

Moreover, Immix is friendly to object pinning.  Semispace and MarkCompact need
to move every single object to make a contiguous region of free space.  Immix,
on the other hand, can allocate into partially occupied blocks by skipping
occupied lines.  Therefore, if an object is pinned, Immix can simply choose not
to evacuate that object in the current GC, while other objects in the same
blocks are still free to move.


# Potential Pinning Parents (PPPs)

Before we enable Immix, we must be aware of one important difference between an
evacuating collector (Immix) and a compacting collector (MarkCompact).

## Racing between moving and pinning

**MarkCompact has a distinct compacting phase after marking, while Immix
traverses the heap only once.**

MarkCompact has the opportunity to pin objects gradually as it traverses through
all live objects during its marking phase.  When MarkCompact starts moving
objects, it already knows which object can be moved, and which objects cannot be
moved.

Evacuating collectors like Immix do not have such an opportunity.  An evacuating
collector moves an object the first time it visits that object.  However, the
order in which the GC visits objects is non-deterministic.

```
Roots --------------> A
  |                   |
  |                   |
  v                   v
  B -----pinning----> C
```

Suppose we have objects A, B and C.  Both A and B point to C, but the field of B
cannot be updated for some reason.

-   If the GC first reaches C from B, it will find that the field of B cannot be
    updated, and has the opportunity to pin C when C has not been moved, yet.
    Then when the GC reaches C from A, C is already pinned, and the GC simply
    skips updating the field of A.
-   But if the GC first reaches C from A, it will move C and update the field in
    A.  When the GC reaches C from B, it will find that C has been moved, but
    the field of B cannot be updated.  This results in an error.
    
This phenomenon demands that we must pin all objects that needs to be pinned
before we traverse the heap.

Then how can we know which objects need to be pinned?  In theory, there are two
basic methods.

The first method is recording all such pinning (un-update-able) edges.  This
requires a write barrier.  If a pinning field is written to, we pin the object
it points to.  However, this method is impractical for CRuby.  CRuby infamously
has ["write-barrier unprotected" (WB-unprotected) objects][wb-unprotected].

[wb-unprotected]: https://blog.heroku.com/incremental-gc

The second method is recording all "potential pinning parents".

## Recording potential pinning parents (PPPs).

A potential pinning parent (PPP) is any object that has un-update-able fields.
In CRuby, it means any object with any field marked with `gc_mark_and_pin`
directly, or indirectly via `rb_gc_mark`, `gc_mark_maybe`, etc.

PPPs include:

-   All objects that use `global_ivtbl`.
-   All Hash that has `compare_by_identity` called.
-   Some `T_IMEMO` types, as discussed above.
-   Some built-in `T_DATA` types.
-   All third-part `T_DATA` types.

You should feel this list familiar.  We have discussed them before.  The first
four cases correspond to the built-in objects that pin their children; the last
case corresponds to legacy third-party C extensions.

To record them, I used a `Vec` (a Rust dynamically-sized array) to hold
references to PPPs.  An object reference is added to that `Vec` when an object
of certain `T_IMEMO` or `T_DATA` types is created, or when an object becomes a
PPP (i.e. when the `Hash#compare_by_identity` method is called, or an object
gains the `FL_EXIVAR` flag).

When GC starts, before traversing the heap, the GC visits the list of all PPPs
and pins their children pointed by their un-update-able fields.   In this way,
while traversing the heap, the GC already knows which objects can be moved and
which objects cannot.

<small>*(Note that the object-pinning mechanism we use here only prevents
objects from moving, but does not keep the objects alive.  Programming languages
usually expose an API (such as the `fixed` keyword for C#) that pins an object
and keeps the object alive, too.  Such a mechanism is usually intended for
passing buffers to C code.)*</small>

When the life and death of all objects are determined, the GC will go through
the PPP list again, and remove all references to dead PPPs.

## Problems with this approach

### Dead PPPs

We recorded all PPPs, but not all PPPs are alive when GC starts.

```
Roots --------------> D
                      |
                      |
                      v
  E -----pinning----> F
```

As shown in the diagram above, E has an un-update-able field pointing to F.
If object E is dead, then F doesn't need to be pinned because it is unnecessary
to update fields of dead objects.  However, if we pin the children of object in
the PPP list as described above, F will still be pinned.

The root problem is that we cannot determine whether E is live of dead until we
finished computing the transitive closure from roots.

<!--
vim: tw=80
-->
