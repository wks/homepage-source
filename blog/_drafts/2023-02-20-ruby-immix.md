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

The real problem is `T_DATA` types defined by third-party C extensions.  Their
types are not required to implement the compacting function.  Even if they
do, the compacting function may contain arbitrary code, and there is no way
to check if it updates every reference fields.  Until we give third-party C
extensions some ways to declare their types never pin any object, we have to
treat all unrecognised `T_DATA` as PPPs.


<!--
vim: tw=80
-->
