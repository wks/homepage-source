---
layout: post
title: "Immix for Ruby"
tags: mmtk ruby
---

Using MMTk, I managed to let the Ruby interpreter use the Immix garbage
collector. Things have not been going smoothly, though.  I am surprised how many
assumptions Ruby developers have made about its garbage collector throughout the
VM, and how bad they are for supporting new GC algorithms.

# Garbage collection, sweeping, compaction and evacuation.

I assume you already know one thing or two about garbage collection.  It is a
form of automatic memory management that reclaims objects when they are no
longer reachable.  There are many garbage collection algorithms out there.
Reference counting, mark-sweep, mark-compact, semispace, Immix, generational
mark-compact, generational Immix, RC-Immix, pauseless, C4, garbage-first (G1),
Shenandoah, ZGC, Sapphire, LXR, to name a few.  Each GC algorithm has a way to
allocate object, identify garbage and reclaim memory.

algorithm    | allocation                          | identification     | reclamation
-------------|-------------------------------------|--------------------|--------------------
naive RC     | free-list                           | reference counting | sweeping to free-list
mark-sweep   | free-list                           | tracing            | sweeping to free-list
mark-compact | bump-pointer                        | tracing            | compaction 
semispace    | bump-pointer                        | tracing            | evacuation
Immix        | bump-pointer<br>+ free-list (block) | tracing            | evacuation<br>+ sweeping (block)

Let's compare mark-sweep, mark-compact and semispace, and ignore naive RC and
Immix for now.  All of them use tracing to identify garbage, and their
difference lies in their allocators.

-   Mark-sweep uses a *free-list allocator*.  Dead objects can be added back to
    the free-list.  It never moves any object.
-   Mark-compact and semispace use a *bump-pointer allocator*.  They need to
    move objects to defragment the heap in order to create a contiguous region
    to bump-allocate into.
    -   Mark-compact does this by *compacting*, that is, moving all live objects
        to one side of the heap so that the rest of the heap is a contiguous
        region of free space.
    -   Semispace does this by *evacuating* all live objects to the other half
        of the heap memory, and freeing the entire old half at once.

That's it.  Mark-sweep, mark-compact and semispace represent the three basic
ways of reclaiming used memory, i.e. *sweeping to free-list*, *compacting* and
*evacuating*.

**What GC algorithm does Ruby use?**

Ruby uses a 

<!--
vim: tw=80
-->
