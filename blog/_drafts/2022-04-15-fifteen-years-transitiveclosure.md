---
layout: post
title: "A Wrong Name: Fifteen Years of TransitiveClosure in MMTk"
tags: mmtk
---

The `TransitiveClosure` interface in MMTk is confusing.  It is an amalgamation
of two different interfaces, and should have been split.  However, no
refactoring happened since it was introduced 15 years ago, when MMTk was still
part of the JikesRVM..., until now.

## MMTk?

I have been contributing to the Memory Management Toolkit (MMTk) project since I
left Huawei.  [MMTk] is a framework for garbage collection.  It was part of the
[JikesRVM], and has been a good platform for GC research.  Many state-of-the-art
garbage collection algorithms have been developed on it.  Now we are
re-implementing MMTk in Rust so that it can be integrated into many different
languages and VMs, such as OpenJDK, V8, Julia, GHC, PyPy and, of course Ruby
which I am working on.

As I started working on MMTk, one part of the code, that is, the
`TransitiveClosure` interface/trait has always confused me.

[MMTk]: https://www.mmtk.io/
[JikesRVM]: https://www.jikesrvm.org/

## TransitiveClosure?

In the core MMTk repository, the [mmtk-core], you can find the
[TransitiveClosure][tc-bad] trait and its implementation for all
`ProcessEdgesWork` instances:

```rust
pub trait TransitiveClosure {
    fn process_edge(&mut self, slot: Address);
    fn process_node(&mut self, object: ObjectReference);
}

impl<T: ProcessEdgesWork> TransitiveClosure for T {
    fn process_edge(&mut self, _slot: Address) {
        unreachable!();
    }
    #[inline]
    fn process_node(&mut self, object: ObjectReference) {
        ProcessEdgesWork::process_node(self, object);
    }
}
```

The presence of `unreachable!();` startled me.  This `impl` item implemented one
method (`process_node`), but declared the other method (`process_edge`)
unreachable.  This is not how we usually use traits.  When we define a trait
with two methods, we expect *both* methods to be callable on *all* of its
instances.  Otherwise, why do we even have the `process_edge` method in
`TransitiveClosure` in the first place?

I guess *some* types must have implemented the `TransitiveClosure` trait *and*
provided a proper `process_edge` implementation.  And...  I am right.  It is
[ObjectsClosure][oc-bad].

```rust
impl<'a, E: ProcessEdgesWork> TransitiveClosure for ObjectsClosure<'a, E> {
    #[inline(always)]
    fn process_edge(&mut self, slot: Address) {
        if self.buffer.is_empty() {
            self.buffer.reserve(E::CAPACITY);
        }
        self.buffer.push(slot);
        // ... more code omitted.
    }
    fn process_node(&mut self, _object: ObjectReference) {
        unreachable!()
    }
}
```

What the [...](https://www.dictionary.com/browse/wtf)!

Clearly `ProcessEdgesWork` and `ObjectsClosure` are implementing two different
interfaces.  `ProcessEdgesWork` only needs `process_node`, while
`ObjectsClosure` only needs `process_edge`.

[mmtk-core]: https://github.com/mmtk/mmtk-core
[tc-bad]: https://github.com/mmtk/mmtk-core/blob/093da769a71067dcd4f37db6f453213e7dace660/src/plan/transitive_closure.rs#L12
[oc-bad]: https://github.com/mmtk/mmtk-core/blob/093da769a71067dcd4f37db6f453213e7dace660/src/plan/transitive_closure.rs#L51

## We should split TransitiveClosure!

Maybe we should split it into two different traits, one contains `process_node`
and the other contains `process_edge`.

Or, should we?

To confirm this, let's find their call sites.

### The process_node method

`process_node` is only called by `XxxSpace::trace_object`, where `XxxSpace` is a
space.  It can be [CopySpace][copy-to], [MallocSpace][ms-to],
[LargeObjectSpace][los-to] and so on.  And `ImmixSpace` has [two][im-to-wm]
[flavours][im-to-om] of `trace_object`.

The `trace_object` method of a space visits an object during tracing.  It marks
or copies the object and, if it is the first time it visits an object, it
*enqueues* the object into the marking queue.  The `process_node` method of
`ProcessEdgesWork` enqueues the object.

[copy-to]: https://github.com/mmtk/mmtk-core/blob/093da769a71067dcd4f37db6f453213e7dace660/src/policy/copyspace.rs#L156
[ms-to]: https://github.com/mmtk/mmtk-core/blob/093da769a71067dcd4f37db6f453213e7dace660/src/policy/mallocspace/global.rs#L244
[los-to]: https://github.com/mmtk/mmtk-core/blob/093da769a71067dcd4f37db6f453213e7dace660/src/policy/largeobjectspace.rs#L170
[im-to-wm]: https://github.com/mmtk/mmtk-core/blob/093da769a71067dcd4f37db6f453213e7dace660/src/policy/immix/immixspace.rs#L317
[im-to-om]: https://github.com/mmtk/mmtk-core/blob/093da769a71067dcd4f37db6f453213e7dace660/src/policy/immix/immixspace.rs#L340

### The process_edge method

`process_edge` is only called by `VM::Scanning::scan_object`.

`scan_object` is implemented by a VM binding (such as [the OpenJDK
binding][openjdk-scanobj] that adds MMTk support for OpenJDK).  MMTk calls
`scan_object` when it needs the VM to find all reference fields in an object for
it.

```rust
fn scan_object<T: TransitiveClosure>(
    trace: &mut T,
    object: ObjectReference,
    tls: VMWorkerThread,
);
```

`scan_object` is called with both the `object` and a call-back (`trace`) as
parameters.  The call-back implements the `TransitiveClosure` trait, and
`scan_object` calls `trace.process_edge` on each *edge*, i.e. each reference
field, of `object`.

[openjdk-scanobj]: https://github.com/mmtk/mmtk-openjdk/blob/86c6f534ae57d03fa45e7a73b698f851c84ab943/mmtk/src/scanning.rs#L42

### Yes!  They are different!

As we confirmed that they are used for two different purposes, let's split them
into two traits.

But my colleague [Yi Lin] reminded me that there were other classes that extends
`TransitiveClosure` in the original MMTk in JikesRVM.  To be safe, I looked into
the history of JikesRVM before making further decisions.

[Yi Lin]: https://qinsoon.com/

## Back in JikesRVM

In [JikesRVM], there is a class named [TransitiveClosure][tc-jikes].  Yes.
It is a *class*, not an interface.

```java
@Uninterruptible
public abstract class TransitiveClosure {
  // Other methods omitted...

  public void processEdge(ObjectReference source, Address slot) {
    VM.assertions.fail("processEdge not implemented.");
  }

  public void processNode(ObjectReference object) {
    VM.assertions.fail("processNode not implemented.");
  }
}
```

Defining `TransitiveClosure` as a class allows it to provide failing default
implementations of `processEdge` and `processNode`.  (Note that the code was
written before Java 8, so interface methods could not have default
implementations.) This allows its subclasses to override one method while
ignoring the other.

[tc-jikes]: https://github.com/JikesRVM/JikesRVM/blob/0b6002e7d746a829d56c90acfc4bb5c560faf634/MMTk/src/org/mmtk/plan/TransitiveClosure.java#L29

### TransitiveClosure subclasses in JikesRVM MMTk

In JikesRVM, there are:

-   TransitiveClosure
    -   TraceLocal
        -   NoGCTraceLocal
        -   MSTraceLocal
        -   SSTraceLocal
        -   MCMarkTraceLocal
        -   MCForwardTraceLocal
        -   ImmixTraceLocal
        -   ImmixDefragTraceLocal
        -   ...
    -   RCZero
    -   RCModifiedProcessor
    -   GenRCModifiedProcessor
    -   ObjectReferenceBuffer
        -   RCDecBuffer
    -   TraceWriteBuffer

### TraceLocal

Back in JikesRVM MMTk, there was no concept of "work packet".  The abstraction
of tracing is the `Trace` class and its thread-local counterpart, `TraceLocal`.
`ThreadLocal` was the local context of a GC thread during tracing GC.

```java
public abstract class TraceLocal extends TransitiveClosure {
    @Override
    @Inline
    public final void processNode(ObjectReference object) {
        values.push(object);
    }

    @Override
    @Inline
    public final void processEdge(ObjectReference source, Address slot) {
        ObjectReference object = VM.activePlan.global().loadObjectReference(slot);
        ObjectReference newObject = traceObject(object, false);
        if (overwriteReferenceDuringTrace()) {
            VM.activePlan.global().storeObjectReference(slot, newObject);
        }
    }

    // ... more code omitted
}
```

`TraceLocal` is the counterpart of both `ProcessEdgesWork` and `ObjectsClosure`
of the current Rust MMTk.  It extends `TransitiveClosure`.  It can process newly
visited objects using `processNode`.  It can also ask the VM to scan objects,
and process reference fields (edges) in `processEdge`.

Each GC algorithm defines its own `TraceLocal` subclass, and some GC algorithms
such as MarkCompact and Immix, have more than one `TraceLocal` for different
kinds of traces.

It looks like the `TransitiveClosure` is a proper interface for `TraceLocal`.
Both `processNode` and `processEdge` are implemented.

However, `TraceLocal` is the only class that implements both `processNode` and
`processEdge`.  There are many other classes that don't.

### Other TransitiveClosure subclasses

Some subclasses of `TransitiveClosure` are related to reference counting (RC).
They are `RCZero`, `RCModifiedProcessor`, etc.  They only override the
`processEdge` method, assuming `processNode` is never called.

What do they do?

They are **field visitors**.  For example, `RCZero` is passed into
`Scanning.scanObject` as the callback.  Its `processEdge` is called for each
field.  What it does is just assign `null` to each field.  Other RC-related
classes are similar.  They record the fields or apply decrements to the objects
each field refers to.

There is also a `TraceWriteBuffer` class.  It only implements `processNode` and
it is used in concurrent mark-sweep.

### So why not introducing a dedicated interface?

There comes an interesting question: *If some classes are just field visitors,
why didn't the developer who introduced them add a dedicated "field visitor"
interface for that?*

To find that out, I dug into the Git revision history of the JikesRVM
repository.

## Through the history

The `git blame` command can show in which commit any line in any source file is
last modified.  I use [vim-fugitive], and it even allows me to follow a line of
code from one commit to another, and see every single change to a piece of code
in history.

[vim-fugitive]: https://github.com/tpope/vim-fugitive

### In the beginning, there was only TraceLocal

[TraceLocal was a root class][tracelocal-beginning].  There was no
`TransitiveClosure` or whatever superclass/interface it could extend/implement.
(`Constants` is an all-static interface, and `Uninterruptible` is just a
marker.)

```java
public abstract class TraceLocal implements Constants, Uninterruptible {
    public final void enqueue(ObjectReference object) throws InlinePragma {
        values.push(object);
    }
    
    public final void traceObjectLocation(Address objLoc, boolean root) throws InlinePragma {
        ObjectReference object = objLoc.loadObjectReference();
        ObjectReference newObject = traceObject(object, root);
        objLoc.store(newObject);
    }

    // ... more code omitted
}
```

Note that `processNode` was named `enqueue`, and `processEdge` was named
`traceObjectLocation`.  Those method names describes *concretely* what those
methods *do*, namely "enqueueing" an object, and "tracing" an object location
(field), not abstractly "processing" a node or an edge.

In those days, everything was built around tracing.

The `Scanning.scanObject` method was built around tracing, too:

```java
public abstract class Scanning implements Constants, Uninterruptible {
    public abstract void scanObject(TraceLocal trace, ObjectReference object);
    // ... more code omitted
}
```

See, `scanObject` takes `TraceLocal` as its parameter.

Let's look into the concrete implementation of `scanObject` for JikesRVM.  (For
some reasons, it was implemented in `org.mmtk.utility.scan.Scan` which didn't
extend `Scanning`.)

```java
public final class Scan implements Uninterruptible {
    public static void scanObject(TraceLocal trace, ObjectReference object) throws InlinePragma {
        MMType type = VM.objectModel.getObjectType(object);
        if (!type.isDelegated()) {
            int references = type.getReferences(object);
            for (int i = 0; i < references; i++) {
                Address slot = type.getSlot(object, i);
                trace.traceObjectLocation(slot);
            }
        } else
            VM.scanning.scanObject(trace, object);
        }
    }
}
```

This code clearly expresses the idea: "Let the `TraceLocal` handle each field."

Yes! **In the beginning, object scanning was tightly coupled with tracing!**
Whenever MMTk scanned an object, it was for only one purpose -- *tracing*, i.e.
finding neighbours, mark/forward the neighbours, and possibly update the object
fields.

Then things changed in 2006.

[tracelocal-beginning]: https://github.com/JikesRVM/JikesRVM/blob/600956237939e61b314535d485dfdfcbab2c0bbe/MMTk/src/org/mmtk/plan/TraceLocal.java#L42

### Then someone introduced field visitor..., err, I mean, "TraceStep"

In 2006, someone created [a commit which introduced a new
version of reference counting][tracestep-commit].

That reference counting algorithm was based on trial deletion.  

```java
public abstract class TraceStep implements Constants, Uninterruptible {
  public abstract void traceObjectLocation(Address objLoc);
}

public abstract class TraceLocal extends TraceStep 
  implements Constants, Uninterruptible {

  public final void traceObjectLocation(Address objLoc)
    throws InlinePragma {
    traceObjectLocation(objLoc, false);
  }
}
```

[tracestep-commit]: https://github.com/JikesRVM/JikesRVM/commit/64f538ca4d348f062f3afb313f519ffcbbbd22bd

## See also

See https://github.com/mmtk/mmtk-core/issues/559

<!--
vim: tw=80
-->
