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
are a blogger like me, you can use Jekyll to render your posts to HTML. If you
only want a little script to count words in a text file, its built-in regular
expression support makes it handy.

If you want to use your favourite C library in Ruby, that's OK, too. You can
implement your Ruby module in C!  Just (1) define your own C struct, (2) define
an `rb_data_type_t`, and (3) define a Ruby class.  Then you call
`TypedData_Make_Struct` and...  Voila! Your shiny new Ruby object backed by a
custom C struct!

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
objects at all?

And why should I care about those VM implementation details?  We are humans, not
machines, aren't we?

# No peaceful world

> 哪有什么岁月静好，不过是有人替你负重前行。
> (There is no such thing as a peaceful world. We feel like so because someone
> take on the burdens.)
>
> *--苏心 (Xin Su)*

Ruby programmers don't need to worry about freeing their objects because the
garbage collector takes care of reclaiming unused objects.

If you instantiate classes implemented in the Ruby language, the object layout
will be the well-known `struct RObject`.  CRuby knows where it holds instance
variables, and the GC can follow those reference fields to traverse the object
graph.

<small>*(I bet you don't know that CRuby sometimes stores instance variables in
a global hash table outside the object.  It's called a "GLOBAL Instance Variable
TaBLe", or `global_ivtbl` for short.)*</small>

If you instantiate *built-in* types implemented in C, CRuby still gets you
covered. CRuby developers have taught the GC how to scan those special objects
by writing functions that [mark their children][gc_mark_children] and [update
their reference fields][gc_update_object_references].

[gc_mark_children]: https://github.com/ruby/ruby/blob/693e4dec236e14432df97010082917a3a48745cb/gc.c#L7242
[gc_update_object_references]: https://github.com/ruby/ruby/blob/693e4dec236e14432df97010082917a3a48745cb/gc.c#L10550

But if you define you own Ruby type backed by your own C struct, CRuby won't
know its layout unless you teach CRuby how to scan it.

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

That's it!  Well, at least that's how we write C extensions 5 years ago.
However, since CRuby [introduced compacting GC in 2019][ruby-compaction], you
should replace `rb_gc_mark` with `rb_gc_mark_movable`, and add an *updating
function* that reassigns each `VALUE` field with the return value of
`rb_gc_location`.

[ruby-compaction]: https://bugs.ruby-lang.org/issues/15626

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

That's confusing.  What exactly do `rb_gc_mark` and `rb_gc_mark_movable` do?
How are they different?  And why do we need an update function too?  I have
heard Python programmers doing "INC" and "DEC" operations all the time.  Why
don't we need them in Ruby?

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

Mark-sweep uses a *free-list allocator*.

-   Free-list allocation is slow, because it needs to find an appropriate slot
    in an appropriate free list.
-   Collection is fast because dead objects can be simply added back to the
    free-list.
-   Mark-sweep never moves object.  It saves time, but can lead to heap
    fragmentation in the long run.  When that happens, the allocator will not be
    able to allocate a slightly bigger object even though there are many small
    gaps between objects.

Mark-compact and semispace use a *bump-pointer allocator*.

-   Bump-pointer allocation is fast, because it only needs to increment a
    cursor.
-   Bump-pointer allocator can only allocate into a contiguous region of free
    space.

Defragmentation makes a contiguous region so that bump-pointer allocators can
allocate into.  It comes at a cost, but the profit of being able to use the fast
bump-pointer allocator compensates for the cost.  It also solves the heap
fragmentation problem.

There are two basic ways of doing defragmentation.

-   Mark-compact does this by *compacting*, that is, moving all live objects to
    one side of the heap so that the rest of the heap is a contiguous region of
    free space.  This is extremely slow because it needs to avoid overwriting
    other live objects while moving objects.
-   Semispace does this by *evacuating* all live objects to the other half of
    the heap memory, and freeing the entire old half at once.  This is faster
    than compacting, but Semispace can only utilise 50% of the total heap space.

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
developers then [introduced compaction][ruby-compaction], making it a hybrid
mark-sweep-compact GC.  Unlike pure mark-compact algorithms, objects are still
allocated from size-segregated free-lists.  It still does mark-sweep GC most of
the time.  But when the user calls `GC.compact`, or if auto-compaction is
enabled, the GC will

1.  Traverse the heap once to mark live object.
2.  Go through all cells with live objects and move the objects, leaving
    "tombstones" (`T_MOVED`) in the original cells.  A tombstone contains a
    "forwarding pointer" to where the original object has been moved to.
3.  Go through all live objects again to update their fields.  If any field
    points to a tombstone (`T_MOVED`), it will be updated to the new address of
    the moved object.
4.  Go through cells that contain tombstones (`T_MOVED`) and reset them as free
    cells.

<small>*I guess the nature of this GC algorithm explains why auto-compaction is
disabled by default, because it allocates objects as slowly as mark-sweep, and
compacts the heap as extremely slowly as mark-compact.*</small>

## GC and Ruby's C extension API

You see, what GC algorithm CRuby uses determines what functions CRuby's C
extension API exposes.  Concretely,

-   When CRuby used mark-sweep, *marking functions* were sufficient.
    -   Inside marking functions, the idiomatic `rb_gc_mark(obj->x);` statement
        gives the GC the field value without updating the field.
-   After compaction was introduced, *updating functions* were needed, too.
    -   Inside updating functions, the `rb_gc_location` function looks up the
        new location of an object, and the idiomatic statement `obj->x =
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

The bad news is, many third-party Ruby libraries were developed before CRuby
introduced compacting GC.  Even today, many of them  still do not provide
updating functions.

<small>*For example, no C-extension types in [this library][liquid-c] have any
updating functions.  See [this][liquid-c1], [this][liquid-c2] and
[this][liquid-c3] type.*</small>

[liquid-c1]: https://github.com/Shopify/liquid-c/blob/0b259bfc259280c76233f2d8c15ad7256abe7e75/ext/liquid_c/document_body.c#L36
[liquid-c2]: https://github.com/Shopify/liquid-c/blob/0b259bfc259280c76233f2d8c15ad7256abe7e75/ext/liquid_c/expression.c#L28
[liquid-c3]: https://github.com/Shopify/liquid-c/blob/0b259bfc259280c76233f2d8c15ad7256abe7e75/ext/liquid_c/block.c#L79

CRuby developers decided to maintain compatibility with those legacy libraries.

1.  The semantics of the publicly exposed `rb_gc_mark` function was changed so
    that it not only marks the object, but also **pins** it.
2.  A new function `rb_gc_mark_movable` was introduced.  It marks the object,
    but does not pin it.

During the marking phase, all objects marked by the existing `rb_gc_mark` call
sites in the legacy libraries are pinned.  Then during compaction, the GC can
move all other objects except those pinned by `rb_gc_mark`.  By doing so, legacy
C extensions can keep working without being aware of object movement, because no
objects they point to ever move.  On the other hand, newer C extensions can
enjoy the benefit of defragmentation by marking the children movable.

Now that we know what kind of GC CRuby is using, we move on to our protagonist,
MMTk.

# MMTk

For those who don't know yet, [MMTk] is a framework for garbage collection.  It
contains abstract components, such as spaces, allocators, tracing, metadata,
etc., for building garbage collectors.  It also has a powerful work packet
scheduler for multi-threaded GC.  Currently, MMTk includes several canonical GC
algorithms, such as MarkSweep, MarkCompact, Semispace and GenCopy, productional
ones such as Immix, GenImmix and StickyImmix, as well as algorithms useful for
debugging, such as NoGC and PageProtect.

[MMTk]: https://www.mmtk.io/

<small>*(Yes. NoGC is a GC algorithm that never reclaims any memory and crashes
when memory is exhausted.  It's known as "epsilon GC" in OpenJDK.)*</small>

MMTk was part of JikesRVM, but is now a VM-independent Rust library.  This makes
MMTk able to profit both VM developers and GC researchers.

-   GC researchers can develop a new GC algorithm on MMTk and try it out on
    multiple VMs, while
-   VMs that use MMTk can gain access to the efficient garbage collection
    algorithms MMTk has now, or will have in the future.

The interface between MMTk and VMs is defined by a bi-directional "VM-binding"
API, making the boundary of MMTk and VM clear and efficient.

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
function to allocate object, and it should just work... well... until the memory
is exhausted.  Despite of this, Angus managed to run a Rails web server with
MMTk and NoGC.

The next step is supporting MarkSweep.  MarkSweep is a non-moving collector, so
we only need to focus on identifying garbage and not worry about pointer
updating at this moment.  The VM needs to implement a "stop-the-world" mechanism
to stop all mutator threads when GC is triggered, a root scanner to visit roots
on the stack as well as in global data structures, and an object scanner that
identifies reference fields in any given object.

The third step is supporting Semispace.  Semispace is a copying collector, and
it copies every single live object during GC.  This step forces us to get object
copying and reference forwarding right.  The VM needs to implement a function to
copy a given object.  If the VM doesn't assume the GC never moves object, this
step should be trivial; but if not, this step could be painful.

The fourth step is supporting GenCopy, the generational copying collector with a
Semispace mature space.  This collector requires a write barrier.  MMTk provides
the write barrier implementation, exposed as
`mmtk::memory_manager::object_reference_write`, and the VM needs to ensure it is
called whenever writing a reference field.  Missing a write barrier invocation
may cause a young object to be erroneously reclaimed.

With stop-the-world, root scanning, object scanning, object copying and write
barriers handled, the VM should be able to use other GC algorithms MMTk
provides, too, such as Immix and GenImmix.  

The fifth step is optimising for fast paths.  "Fast paths" here refer to the
most commonly executed code paths of allocation and write barriers.  For
example, the fast path of a bump-pointer allocation is incrementing the cursor
and checking whether it has exceeded a given limit.  If it has, it should jump
to the slow path which is usually implemented out of line as a function.
Because the fast paths are executed very often, VMs should JIT-compile and
inline the fast paths of object allocation and write barriers... if they care
about performance.


# MMTk and CRuby

We have discussed what MMTk is, and how to let a VM use MMTk.

Let's use MMTk for CRuby, and make all of its GC algorithms available for CRuby!

Unfortunately, there is a catch.  CRuby needs object pinning, and that limits
what GC algorithms we can use.

## CRuby and object pinning

Why does CRuby pin objects?

### Conservative stack scanning

CRuby is, as the name suggests, written in C.  CRuby implements some Ruby
functions using C functions, and may hold references to Ruby objects in C local
variables (the `VALUE` type).  Those are direct pointers to Ruby heap objects,
without any indirection tables (like OpenJDK and Lua) or reference counting
(like CPython).

Here comes the problem: C compilers cannot generate stack maps
(the metadata that records "which offsets of a frame contain references").  How
can the GC find object references on the stack?

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

-   Any object that uses "global instance variable table" (`global_ivtbl`).
-   Any `Hash` instance that has the `compare_by_identity` method called.
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

Other objects pin their children solely for historical reasons.  Examples
include `imemo_iseq` and the `global_ivtbl`.  We will later show that they don't
have to pin their children.  Presumably, CRuby developers have full control over
those built-in-types, and they can apply `rb_gc_mark_movable` and
`rb_gc_location` to those types as long as copying GC was introduced.  In
reality, this didn't happen.  I think it is related to the way compaction is
used.  Auto-compaction is not enabled by default.  Manual compaction is usually
called only once before forking a Ruby VM to reduce its memory footprint.  In
such a use case, it will not be profitable to make every built-in type support
object movement. 

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
-   All `Hash` instances that have the `compare_by_identity` method called.
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

## Dead PPPs

One problem with this approach is that the PPP list may contain dead objects.

We recorded all PPPs when they are created, but when GC starts, not all recorded
PPPs are still alive.  This will result in pinning objects that no longer needs
to be pinned.  Let's see an example:

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
finished computing the transitive closure from roots.  When using MarkCompact,
we traverse the heap twice.  We pin children of PPPs as we traverse the heap in
during the non-moving marking phase.  By doing so, we pin only children of live
PPPs.  We don't have the same opportunity for Immix because we only traverse the
heap once.

One obvious "solution" is doing an additional non-moving heap traversal before
moving objects.  That's unnecessarily costly, and will ruin all the performance
advantage of Immix.

But I think it will not be that bad to conservatively pin the children of dead
PPPs.  If a PPP is dead, it will pin its children in this GC, but it will also
be removed from the PPP list after the current GC.  Therefore, during the next
GC, those conservatively pinned children will no longer be pinned again.

## Reducing the number of PPPs

Given that this approach is conservative, the best things we can do are

1.  Reduce the number of PPPs, and
2.  Reduce the number of (both pinning and non-pinning) fields of PPPs.

In addition to `global_ivtbl` and `Hash`, other PPPs are instances of `T_IMEMO`
and `T_DATA`.  It is worth knowing which types appear the most frequently among
PPPs.

### The most frequent PPP types

Before I actually enabled copying GC, I did some experiments.  I ran a simple
program shown below and recorded objects of PPP types.

```ruby
puts "Hello world!"
GC::start
puts "Goodbye world!"
```

Note: This program will trigger GC twice, once during start-up, and the other
time for `GC::start`.  During start-up, the VM also triggers a GC because the
parser is allocating objects.

When?     | total PPPs | total PPP edges | pinning edges | unique pinned objects | live PPPs after GC
---       | ---        | ---             | ---           | ---                   | ---
start-up  | 455        | 1086            | 285           | 115                   | 455/455
manual    | 1319       | 2206            | 107           | 64                    | 1080/1319

It looks like during start-up there are fewer PPP instances, but pinning more
objects; during execution, more PPPs are created but they have less pinning
edges and pin less objects.

The following table shows the type of PPPs during start-up.

<small>*(The data is collected in a different run, so the numbers of instances
do not sum up to 455.)*</small>

kind      | type                                   | instances | instances that pin any child
---       | ---                                    | ---       | ---
`T_DATA`  | `mutex`                                | 1         | 0
`T_DATA`  | `encoding`                             | 12        | 0
`T_DATA`  | `(null)`                               | 1         | 1
`T_DATA`  | `ENV`                                  | 1         | 0
`T_DATA`  | `ARGF`                                 | 1         | 1
`T_IMEMO` | `iseq`                                 | 177       | 0
`T_DATA`  | `ractor`                               | 1         | 1
`T_DATA`  | `VM`                                   | 1         | 0
`T_DATA`  | `VM/thread`                            | 1         | 1
`T_DATA`  | `binding`                              | 1         | 1
`T_DATA`  | `thgroup`                              | 1         | 0
`T_DATA`  | `memory_view/exported_object_registry` | 1         | 0
`T_DATA`  | `parser`                               | 16        | 16
`T_IMEMO` | `ast`                                  | 15        | 14
`T_IMEMO` | `tmpbuf`                               | 147       | 0
`T_IMEMO` | `parser_strterm`                       | 109       | 0

*Note: the `Data:(null)` is an old-style un-typed Data. It is a
`mark_marshal_compat_t`.*

From this table, we see some PPP types have many instances, but hardly pin any
objects.  For example, there are 177 `iseq` instances, but none of them pin any
children.

The following table lists top PPP instances sorted by the number of fields.

PPP                            | total fields | pinning fields
---                            | ---          | ---
0x200ffc6b6b0 (Data:VM/thread) | 143          | 142
0x200ffcc5638 (Memo:ast)       | 54           | 0
0x200ffc703f8 (Memo:iseq)      | 31           | 0
0x200ffca4330 (Memo:iseq)      | 25           | 0
0x200ffc970b0 (Memo:iseq)      | 20           | 0
0x200ffc7b020 (Memo:iseq)      | 19           | 0
0x200ffcaaf08 (Memo:iseq)      | 16           | 0
0x200ffccbd78 (Memo:iseq)      | 13           | 0
0x200ffc97430 (Memo:iseq)      | 13           | 0
0x200ffc0ae28 (Data:(null))    | 13           | 13
0x200ffc96908 (Memo:iseq)      | 11           | 0
0x200ffcc5e18 (Memo:iseq)      | 10           | 0
0x200ffc9faf0 (Memo:iseq)      | 10           | 0
0x200ffcab288 (Memo:iseq)      | 9            | 0
0x200ffc90200 (Memo:iseq)      | 8            | 0
0x200ffcc5fd8 (Memo:iseq)      | 7            | 0
0x200ffcc7ac0 (Memo:iseq)      | 7            | 0
0x200ffccff18 (Memo:iseq)      | 7            | 0
0x200ffc70388 (Memo:iseq)      | 7            | 0
0x200ffcc6048 (Memo:iseq)      | 6            | 0
0x200ffccad48 (Memo:iseq)      | 6            | 0
0x200ffccb9f8 (Memo:iseq)      | 6            | 0
0x200ffca15b0 (Memo:iseq)      | 6            | 0
0x200ffca26f8 (Memo:iseq)      | 6            | 0
0x200ffc93f08 (Memo:iseq)      | 6            | 0
0x200ffc974a0 (Memo:iseq)      | 6            | 0
0x200ffc7a798 (Memo:iseq)      | 6            | 0
0x200ffcc56e0 (Data:parser)    | 5            | 5
0x200ffca2848 (Memo:iseq)      | 5            | 0
0x200ffca28b8 (Memo:iseq)      | 5            | 0

The number of "total fields" and "pinning fields" are collected from
`gc_mark_children`.  "Pinning fields" is the number of fields visited by
`rb_gc_mark`; "total fields" includes that plus other fields visited by
`rb_gc_mark_movable`.

We see that `iseq` objects tend to have many non-pinning fields but no pinning
field.  But `iseq` is really a PPP type because it has fields that could be
visited by the pinning `rb_gc_mark` function under certain condition, such as
this one:

```c
    if (FL_TEST_RAW((VALUE)iseq, ISEQ_NOT_LOADED_YET)) {
        rb_gc_mark(iseq->aux.loader.obj);
    }
```

Maybe this condition is seldom met.  Maybe the field usually contains `NULL` or
`nil`.  But as long as this `rb_gc_mark` exists, we must treat the `iseq` type
as "potentially pinning".

**Types like `iseq` have the highest priority to be eliminated from the list of
PPP types.**  It has many instance, therefore each of them needs to be scanned
before every GC (`iseq` instances are quite long-living).  It has many
non-pinning fields, which means every invocation of `gc_mark_children` is
expensive, as it has to visit many irrelevant fields.  We rely on
`gc_mark_children` to find its pinning fields, but we can't control which field
`gc_mark_children` visits unless we rewrite the marking function for `iseq` by
hand.

### Eliminating PPP types

Our collaborator Peter from Shopify examined the marking function of `iseq` and
found that all of its fields should be update-able.  He made a series of changes
to the marking and updating function for `iseq`, and [the last of
them][iseq-movable] made it possible to update all fields of `iseq`.  From then
on, `iseq` is no longer a PPP type.

[iseq-movable]: https://github.com/ruby/ruby/pull/7156

After that change, the number of total number of PPPs during start-up is greatly
reduced.  More importantly, the number of live PPPs after the first GC is also
greatly reduced.

<small>*Note: (1) The data is collected in another run, so the numbers will not
be consistent with previous tables. (2) The number of pinning edges fluctuates
slightly between execution.*</small>

Before/after | total PPPs | total PPP edges | pinning edges | unique pinned objects | live PPPs after GC
---          | ---        | ---             | ---           | ---                   | ---
Before       | 457        | 1257            | 277           | 115                   | 215/455
After        | 280        | 325             | 278           | 115                   | 41/280

I also [made a change][givtbl-movable] to CRuby to make all instance variables
held in `global_ivtbl` update-able.

[givtbl-movable]: https://github.com/ruby/ruby/pull/7206

### Making field updating easier

CRuby has many built-in `T_IMEMO` and `T_DATA` types, and some of them still
don't support field updating now.  In theory, it is possible to make all such
types capable to update their fields, as long as the fields are not conservative
like `imemo_tmpbuf`.  Doing this thoroughly and systematically takes time and
effort.

Despite of this, our collaborator Peter also [added a new
function][rb_gc_mark_and_move], `rb_gc_mark_and_move`, as an alternative to
calling `rb_gc_mark`, `rb_gc_mark_movable` and `rb_gc_location` directly:

```c
void rb_gc_mark_and_move(VALUE *ptr);
```

[rb_gc_mark_and_move]: https://github.com/ruby/ruby/pull/7140

This function visits the field `*ptr`.  During the marking phase, it will read
from the field; during compaction, it will update the content of the field.
Using this function, it is possible to write one function, to handle both the
marking and the updating of a given type.  The `rb_iseq_mark_and_update`
function introduced in [the pull request][rb_gc_mark_and_move] is one example.
With it, both `gc_mark_imemo` and `gc_ref_update_imemo` call the same function
`rb_iseq_mark_and_update` instead of calling `rb_iseq_mark` and
`rb_iseq_update_references` respectively.

When       | function (before)           | function (after)
---        | ---                         | ---
marking    | `rb_iseq_mark`              | `rb_iseq_mark_and_update`
compacting | `rb_iseq_update_references` | `rb_iseq_mark_and_update`

This not only makes it easier to correctly implement types that support field
updating, but also makes MMTk integration easier.  Because `rb_gc_mark_and_move`
takes the *address* of the field (instead of value) as parameter, MMTk can
intercept this function, and let the plan (GC algorithm) decide whether to read
from field or update it as well.

### Declarative marking

Another alternative to writing marking and updating functions is using
**declarative marking**.

What the GC really need is identifying the *offsets* of reference fields of a
give object.  We can give each type a list of offsets so that the GC can read
and update fields at the given offsets.  The list can also be compressed into a
bitmap.  VMs of statically typed languages, such as Java, usually have the
capability of generating such lists (known as "object map") for their classes.

Our collaborator Matt is working on [declarative marking][decl-mark-pr] for
`rb_typed_data_struct`.  The developer provides an array to declare reference
fields, for example:

```c
static const size_t enumerator_refs[] = {
    RUBY_REF_EDGE(enumerator, obj),
    RUBY_REF_EDGE(enumerator, args),
    RUBY_REF_EDGE(enumerator, fib),
    RUBY_REF_EDGE(enumerator, dst),
    RUBY_REF_EDGE(enumerator, lookahead),
    RUBY_REF_EDGE(enumerator, feedvalue),
    RUBY_REF_EDGE(enumerator, stop_exc),
    RUBY_REF_EDGE(enumerator, size),
    RUBY_REF_EDGE(enumerator, procs),
    RUBY_REF_END
};
```

Then the `rb_typed_data_struct` no longer needs to provide the marking and
compacting functions.

```c
static const rb_data_type_t enumerator_data_type = {
    "enumerator",
    {
        NULL,                   // No marking function needed.
        enumerator_free,
        enumerator_memsize,
        NULL,                   // No compacting function needed.
    },
    0,
    (void *)enumerator_refs,
    RUBY_TYPED_FREE_IMMEDIATELY
    | RUBY_TYPED_DECL_MARKING   // This flag indicates declarative marking
};
```

[decl-mark-pr]: https://github.com/ruby/ruby/pull/7153

This approach has a limitation, though.  It is not suitable for handling
dynamically-sized objects, such as arrays.  It is also not suitable if the type
contains union fields which sometimes hold references and sometimes hold plain
values.

### What about other PPP types?

A `Hash` instance pins its keys if it compares by identity.  It is because the
current implementation uses object addresses as keys.  A wiser strategy is using
**address-based hashing**.  It uses the object address as hash code, but when
the object is moved, the GC saves the old address in a hidden field of the new
object, and use the saved address as its "identity hash".  This preserves the
"identity hash" of an object across copying GC.

The real problem is `T_DATA` types defined by third-party C extensions.  Those
types are not required to implement the compacting function.  Even if they
do, the compacting function may contain arbitrary code, and there is no way
to check if it updates every reference fields.  Until we give third-party C
extensions some ways to declare their types never pin any object, we have to
treat all unrecognised `T_DATA` as PPPs.


# Enabling copying GC

With potential pinning parents (PPPs) recorded, we can give Immix a try.
Compared with MarkSweep, we need to do more things.

Before GC, we pin all objects pointed by un-update-able fields of recorded PPPs,
regardless whether the PPPs are live or dead.

When scanning machine stacks conservatively, we pin all objects pointed by
conservative roots.

When scanning an object, we call both `gc_mark_children` and
`gc_update_object_references`.  This looks redundant, but it is necessary.
Consider the following example,

```c
struct Foo {
    VALUE x;
    VALUE y;
}

void foo_mark(struct Foo *obj) {
    rb_gc_mark_movable(obj->x);
    rb_gc_mark(obj->y);
}

void foo_update(struct Foo *obj) {
    obj->x = rb_gc_location(obj->x);
    // skipped y
}

const rb_data_type_t foo_data_type = { ... };
```

The marking function pins `y`, therefore the updating function can skip `y`
because it will not be moved.  It work well for mark-compact, but when using an
evacuating GC, it is insufficient to call either of them alone.

-   If we call `foo_mark` alone (via `gc_mark_children`), we can't update the
    update-able field `x`;
-   If we call `foo_update` alone (via `gc_update_object_references`), we can't
    visit the field `y`, and the children of `y` will be considered dead if it
    cannot be reached from elsewhere.

Therefore it is necessary to call both `gc_mark_children` and
`gc_update_object_references`.

-   Calling `gc_mark_children` ensures all reference fields.
-   Calling `gc_update_object_references` ensures all reference fields that can
    be updated are updated.

And we intercept `rb_gc_mark`, `rb_gc_mark_movable` and `rb_gc_location` so that
all of them call the `trace_object` function in the MMTk core.

**`trace_object`** is perhaps the most important function in a tracing garbage
collector.  It visits an object.  If it is the first time the object is visited,
it will mark or evacuate it, depending on the GC algorithm.  It return its new
address (if moved).

-   `rb_gc_location` will return the return value of `trace_object`, i.e. the
    new address of the object.
-   `rb_gc_mark` simply calls `trace_object` and ignores the return value
    because the child object is already pinned.
    -   *(Remember that any object that marks its field with `rb_gc_mark` is a
        PPP.)*
-   `rb_gc_mark_movable` simply calls `trace_object` and ignores the return
    value, too, because we know we will call `gc_update_object_references`
    later.

By wiring CRuby's marking/updating functions to MMTk's `trace_object`, I expect
that Immix should work.  But it still doesn't.

## Assertions during marking

CRuby performs various assertions about object consistency during the marking
phase.  Here is one example:

```c
cc_table_mark_i(ID id, VALUE ccs_ptr, void *data_ptr)
{
    struct cc_tbl_i_data *data = data_ptr;
    struct rb_class_cc_entries *ccs = (struct rb_class_cc_entries *)ccs_ptr;
    VM_ASSERT(vm_ccs_p(ccs));
    VM_ASSERT(id == ccs->cme->called_id);       // ERROR

    if (METHOD_ENTRY_INVALIDATED(ccs->cme)) {
        rb_vm_ccs_free(ccs);                    // ERROR
        return ID_TABLE_DELETE;
    }
    else {
        gc_mark(data->objspace, (VALUE)ccs->cme);

        for (int i=0; i<ccs->len; i++) {
            VM_ASSERT(data->klass == ccs->entries[i].cc->klass);        // ERROR
            VM_ASSERT(vm_cc_check_cme(ccs->entries[i].cc, ccs->cme));   // ERROR

            gc_mark(data->objspace, (VALUE)ccs->entries[i].ci);
            gc_mark(data->objspace, (VALUE)ccs->entries[i].cc);
        }
        return ID_TABLE_CONTINUE;
    }
}
```

Let's ignore `rb_vm_ccs_free` for now.

Three of the assertions in this function only works if the GC doesn't move
objects.

-   `VM_ASSERT(id == ccs->cme->called_id)`;
-   `VM_ASSERT(data->klass == ccs->entries[i].cc->klass);`
-   `VM_ASSERT(vm_cc_check_cme(ccs->entries[i].cc, ccs->cme));`

They will lead to assertion failures or even crashes because they access the
children of the current object (the class or module object that owns the current
`ccs`), and the children may have been moved.

`ccs->cme` and `ccs->entries[i].cc` may point to another heap object.  Remember
that Immix moves objects when visiting an object for the first time.  If the
object pointed by `ccs->entries[i].cc` was visited before, the GC would have
copied the content of that object to a new location, leaving a "tombstone" (a
forwarding address) at its original location.  This will overwrite some of the
object's fields.  The `cc->klass` field may have been overwritten, and it is not
safe for the application to inspect its content.

There are other functions in CRuby that do assertions on children during GC,
such as [`gc_mark_imemo` for `imemo_env`][gc-mark-imemo-assert] and
[`VM_ENV_ENVVAL`][vm-env-envval-assert].  I have to disable those assertions
[one][disable-assertion1] by [one][disable-assertion2].

[gc-mark-imemo-assert]: https://github.com/ruby/ruby/blob/dc33d32f12689dc5f29ba7bf7bb0c870647ca776/gc.c#L7202
[vm-env-envval-assert]: https://github.com/ruby/ruby/blob/dc33d32f12689dc5f29ba7bf7bb0c870647ca776/vm_core.h#L1404
[disable-assertion1]: https://github.com/mmtk/ruby/commit/364e91963354ef20b3f02323880a2a13f8043d20
[disable-assertion2]: https://github.com/mmtk/ruby/commit/cd03778b79700cdbca93fbcb733a71d19267c25a

In theory, the GC is allowed to erase the entire from-space object when it is
moved.  For this reason, we should remove all assertions that access any
children of an object during GC.

## Cleaning-up operations during marking

The function call `rb_vm_ccs_free(ccs);` mentioned in the last section is
intended to clean up the `ccs` when invalidated.  It accesses
`ccs->entries[i].cc`, too, and it will crash if `cc` has been moved.

But I also think it is abusing the marking phase of GC to clean up non-memory
resources.  It is the responsibility of the **finalization** mechanism to clean
up such resources.


# Results

The code is hosted on GitHub.

-   mmtk-ruby: https://github.com/mmtk/mmtk-ruby
-   ruby: https://github.com/mmtk/ruby

You can give it a try by cloning those repositories and following the
[README.md][mmtk-ruby-readme] file in the `mmtk/mmtk-ruby` repository.

[mmtk-ruby-readme]: https://github.com/mmtk/mmtk-ruby/blob/master/README.md

So far, I managed to let `miniruby` pass all the bootstrap tests.  You can try
it by running:

```bash
make btest RUN_OPTS=--mmtk-plan=Immix
```

# Lessons learned

Probably the worst thing a VM can do about GC is making too many assumptions
about the GC algorithm it is using.

CRuby started with mark-sweep.  Mark-sweep never moves objects.  Because of
this, it is sufficient to read from reference fields of objects without updating
them during GC.  It is safe to navigate through object references to do
assertions on the children of an object, or doing clean-up work during GC.

What's worse, assumptions can leak.  The `dmark` function is part of the C
extension API.  It allows third-party extensions to execute arbitrary code in
it, with the only obligation to call `rb_gc_mark` on the *values* of its
reference fields.

While it worked well when CRuby used mark-sweep, it made supporting other GC
algorithms difficult.

Later, CRuby introduced compaction, but much of the code in CRuby core and all
third-party C extensions already made the assumption that objects never move and
a `dmark` function is sufficient.  CRuby had to work around this by introducing
object pinning, and hiding object pinning underneath the `rb_gc_mark` function.

And the assumption of "GC has a marking phase and a compaction phase" also
leaked.  Now both the `dmark` and the `dcompact` functions are exposed to
third-party C extensions.  The `dcompact` function is obliged to update some but
not all of its reference fields with `rb_gc_location`, skipping pinned fields.

Later, I introduced Immix, an evacuating GC, to CRuby.  And I had to work this
around by calling both `gc_mark_children` and `gc_update_object_references` on
every object.  I also had to identify "potential pinning parents" to work around
objects that cannot update some of its fields.

Why do we end up with so many quirks?  Can't CRuby just let `T_DATA` provide a
general-purpose field visitor that visits fields with something like
`rb_gc_mark_and_move(VALUE*)`, or use "declarative marking" in the first place?

Well, I guess Ruby wasn't designed for those who knows "By doing this, the
machine will run fast, run more effectively, or something something something."
If "calling `rb_gc_mark` on reference fields" is easy enough for average
programmers to understand, they will probably be happy with it, and such an API
would probably work well from 1995 to 2019.

But, as [this paper][JBH11] pointed out, *language implementers who are serious
about correctness and performance need to understand deferred gratification*.
If CRuby had a better GC interface, it would have been much easier to support
other GC algorithms.  Implementing a high-performance VM requires much
expertise.  Even if the VM developers want to make average programmers happy,
they should design the API in such a way that it is easy to do things right, and
hard (if possible at all) to do things wrong.

[JBH11]: https://users.cecs.anu.edu.au/~steveb/pubs/papers/php-mspc-2011.pdf

But I still feel lucky that CRuby didn't start with naive reference counting
(RC) like CPython did.  If CRuby used naive RC, the performance would be
hopeless as the INC and DEC operations would dominate the execution time [like
in Swift][UGF17].  And it would be impossible to even migrate to mark-sweep,
and get stuck with naive RC [like Pyston][pyston-0.5].  The choice of
conservative stack scanning is also lucky because we found ways to reach
remarkable performance with [RC-Immix and conservative stack scanning][SBM14].

[UGF17]: https://dl.acm.org/doi/10.1145/3133841.3133843
[pyston-0.5]: https://blog.pyston.org/2016/05/25/pyston-0-5-released/
[SBM14]: https://users.cecs.anu.edu.au/~steveb/pubs/papers/consrc-oopsla-2014.pdf

Anyway, CRuby still has hope.  Let's keep working on it.

{% comment %}
vim: tw=80
{% endcomment %}
