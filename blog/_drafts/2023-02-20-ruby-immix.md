---
layout: post
title: "Immix for Ruby"
tags: mmtk ruby
---

I have been working on MMTk support for CRuby (the official Ruby interpreter,
written in C, as opposite to JRuby, TruffleRuby, etc.).  I managed to let CRuby
use the Immix garbage collector via MMTk.  It is the first time CRuby can run
with an evacuating copying collector.  Isn't that exciting?  In this article,
I'll introduce my experience of getting MMTk and Immix working, and the
challenges I encountered. I am surprised by how many assumptions CRuby
developers have made about its garbage collector throughout the VM
implementation, and how bad they are for supporting new GC algorithms.

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

Ruby, a language for human, has attracted millions of programmers around the
world with its elegant syntax and extensibility.  If you want a dynamic web
site, you can prototype it in an hour using the Ruby on Rails framework.  If you
only want a little script to count words in a text file, its built-in regular
expression support makes it handy.

If you want to use your favourite C library in Ruby, that's OK, too. You can
implement your Ruby module in C!  Just define a Ruby class, define an
`rb_data_type_t`, and define a C struct.  Then you call `TypedData_Make_Struct`
and...  Voila! Your shiny new Ruby object backed by a C struct!

You may even be tempted to race [your old pure Ruby module][liquid] against
[your C version][liquid-c], and the C version [probably
wins][liquid-c-performance].

Oh, don't forget to teach Ruby how to do garbage collection for your C type
using the `rb_data_type_t` structure you just defined.  Teach the GC how to mark
its `VALUE` fields, how to forward the `VALUE` fields when the GC moves object,
and...

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

If you instantiate classes written in Ruby, the Ruby runtime knows where
instance variables are held, and the GC can traverse from the object to its
children.

If you instantiate *built-in* types implemented in C, Ruby still gets you
covered. CRuby developers have taught Ruby how to scan those objects by writing
functions that [mark their children][gc_mark_children] and [update their
reference fields][gc_update_object_references].  Other Ruby implementations
(such as JRuby, TruffleRuby, etc.) also have their own way to scan objects.

[gc_mark_children]: https://github.com/ruby/ruby/blob/693e4dec236e14432df97010082917a3a48745cb/gc.c#L7242
[gc_update_object_references]: https://github.com/ruby/ruby/blob/693e4dec236e14432df97010082917a3a48745cb/gc.c#L10550

But if you define you own Ruby type backed by a C struct, you have to teach
CRuby how to scan them.

How?

You write a marking function that calls `rb_gc_mark` on each of the `VALUE`
fields.

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

That's it... err... if you write your C extension module 10 years ago.  But
since CRuby introduced compacting GC, you should replace `rb_gc_mark` with
`rb_gc_mark_movable`, and add an updating function that reassigns each `VALUE`
field with the return value of `rb_gc_location`.

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
time.  Why is Ruby different?

To understand these issues, we need to understand the differences between GC
algorithms.


# Sweeping, compacting and evacuating garbage collectors

I assume you already know one thing or two about garbage collection (GC).  It is
a form of automatic memory management that reclaims objects when they are no
longer reachable.  There are many garbage collection algorithms out there.
Reference counting (RC), mark-sweep, mark-compact, semispace, Immix,
generational mark-compact, generational Immix, RC-Immix, pauseless, C4,
garbage-first (G1), Shenandoah, ZGC, Sapphire, LXR, to name a few.  Each GC
algorithm has a way to allocate object, identify garbage and reclaim memory.

algorithm    | allocation                          | identification      | reclamation
-------------|-------------------------------------|---------------------|--------------------
naive RC     | free-list                           | reference counting  | sweeping to free-list
mark-sweep   | free-list                           | tracing             | sweeping to free-list
mark-compact | bump-pointer                        | tracing             | compaction 
semispace    | bump-pointer                        | tracing             | evacuation
Immix        | bump-pointer<br>+ free-list (block) | tracing             | evacuation<br>+ sweeping (block/line)

Let's compare mark-sweep, mark-compact and semispace, and ignore naive RC and
Immix for now.  All of them use tracing to identify garbage, and their
difference lies in their allocators.

-   Mark-sweep uses a *free-list allocator*.  Free-list allocation is slow, but
    dead objects can be simply added back to the free-list, which is fast.  It
    never moves any object.
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
is simple.  It traverses the heap once to find live objects.  All objects that
are not reached are dead.  The GC then looks all cells that have been allocated,
and calls `obj_free` to clean up dead object, and reclaim the cell space.

Later, people probably found that heap fragmentation is a problem.  CRuby
developers introduced compaction, making it a hybrid mark-sweep and mark-compact
GC.  Unlike pure mark-compact algorithms, objects are still allocated from
size-segregated free-lists.  It still does mark-sweep GC most of the time.  But
when the user calls `GC.compact`, or if auto-compaction is enabled, the GC will

1.  traverse the heap once to mark live object, and
2.  traverse the heap again to move live objects together.

Then large chunks of memory can be returned to the operating system.

<small>*I guess the nature of this GC algorithm explains why auto-compaction is
disabled by default, because it allocates objects as slowly as mark-sweep, and
compacts the heap as extremely slowly as mark-compact.*</small>

## GC and C extension API

You see.  The C extension API requires the developers to write their marking and
updating functions because of the GC algorithm CRuby uses.

When CRuby used mark-sweep, a marking function is sufficient.  It never update
any fields.

After compaction is introduced, C extension developers need to add an updating
function.  That's because when objects are moved, the pointers in the fields
need to be updated to point to the new locations of the field.  The
`rb_gc_location` function gets the new location of an object, and the idiomatic
`obj->x = rb_gc_location(obj->x);` statement updates the field by assigning the
return value back to the field.

<!--
vim: tw=80
-->
