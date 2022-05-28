---
layout: post
title: "A Wrong Name: Fifteen Years of TransitiveClosure in MMTk"
tags: mmtk
---

The `TransitiveClosure` interface in MMTk is confusing.  It should have been
split into two different interfaces, but not... until now.  What's more
interesting is how we ended up having an interface like that 15 years ago, and
why it stayed that way since then.

## MMTk?

I have been contributing to the Memory Management Toolkit (MMTk) project since I
left Huawei.  [MMTk] is a framework for garbage collection.  It was part of the
[JikesRVM], and has been a good platform for GC research.  Many state-of-the-art
garbage collection algorithms have been developed on it.  Now we are
re-implementing MMTk in Rust so that it can be integrated into many different
languages and VMs, such as OpenJDK, V8, Julia, GHC, PyPy and, of course
[Ruby][mmtk-ruby] which I am working on.

As I started working on MMTk, one part of the code, that is, the
`TransitiveClosure` interface/trait has always confused me.

[MMTk]: https://www.mmtk.io/
[JikesRVM]: https://www.jikesrvm.org/
[mmtk-ruby]: https://github.com/mmtk/mmtk-ruby/

## TransitiveClosure?

In the core MMTk repository, the [mmtk-core], you can find the
[TransitiveClosure][tc-bad] trait and its implementation for all
`ProcessEdgesWork` instances. (Let's not worry about what `ProcessEdgesWork`
does for now.)

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

The presence of `unreachable!();` startled me.  The code implemented one method
(`process_node`), but declared the other method (`process_edge`) unreachable.
This is not how we usually use traits.  When we define a trait with two methods,
we expect *both* methods to be callable on *all* instances. Otherwise, why do we
even have the `process_edge` method in `TransitiveClosure` in the first place?

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

`unreachable!()` again?  What the [...](https://www.dictionary.com/browse/wtf)!

Clearly `ProcessEdgesWork` and `ObjectsClosure` are implementing two different
interfaces.

-   `ProcessEdgesWork` only implements `process_node`, while
-   `ObjectsClosure` only implements `process_edge`.

[mmtk-core]: https://github.com/mmtk/mmtk-core
[tc-bad]: https://github.com/mmtk/mmtk-core/blob/093da769a71067dcd4f37db6f453213e7dace660/src/plan/transitive_closure.rs#L12
[oc-bad]: https://github.com/mmtk/mmtk-core/blob/093da769a71067dcd4f37db6f453213e7dace660/src/plan/transitive_closure.rs#L51
[wtf]: https://www.dictionary.com/browse/wtf

## Should we split TransitiveClosure?

Maybe we should split it into *two different traits*, one contains
`process_node` and the other contains `process_edge`.  In this way, a type may
only implement the trait it needs, and not the `unreachable!()` stub.

Or, should we?

To confirm this, let's find their call sites, and see whether we ever use both
methods at the same time.  The short answer is, no.

### The process_node method

`process_node` is only called by `XxxSpace::trace_object`, where `XxxSpace` is a
concrete space.  It can be [CopySpace][copy-to], [MallocSpace][ms-to],
[LargeObjectSpace][los-to] and so on.

<small>*`ImmixSpace` even has [two][im-to-wm] [flavours][im-to-om] of
`trace_object`, both call `process_node`.*</small>

The `trace_object` method of a space visits an object during tracing.  It marks
or copies the object and, if it is the first time it visits an object, it
*enqueues* the object into the marking queue by calling the `process_node`
method which does the actual enqueuing.

[copy-to]: https://github.com/mmtk/mmtk-core/blob/093da769a71067dcd4f37db6f453213e7dace660/src/policy/copyspace.rs#L156
[ms-to]: https://github.com/mmtk/mmtk-core/blob/093da769a71067dcd4f37db6f453213e7dace660/src/policy/mallocspace/global.rs#L244
[los-to]: https://github.com/mmtk/mmtk-core/blob/093da769a71067dcd4f37db6f453213e7dace660/src/policy/largeobjectspace.rs#L170
[im-to-wm]: https://github.com/mmtk/mmtk-core/blob/093da769a71067dcd4f37db6f453213e7dace660/src/policy/immix/immixspace.rs#L317
[im-to-om]: https://github.com/mmtk/mmtk-core/blob/093da769a71067dcd4f37db6f453213e7dace660/src/policy/immix/immixspace.rs#L340

### The process_edge method

`process_edge` is only called by `VM::Scanning::scan_object`.

`scan_object` is implemented by a VM binding (such as [the OpenJDK
binding][openjdk-scanobj]) because it is VM-specific.  MMTk calls `scan_object`
when it needs the VM to locate all reference fields in an object.

```rust
pub trait Scanning<VM: VMBinding> {
    fn scan_object<T: TransitiveClosure>(
        trace: &mut T,
        object: ObjectReference,
        tls: VMWorkerThread,
    );

    // ... more code omitted
}
```

The `trait` parameter is a call-back.  When `scan_object(trace, object, tls)` is
called, it scans `object`, and calls `trace.process_edge` on each edge, i.e.
each reference field, of `object`.

[openjdk-scanobj]: https://github.com/mmtk/mmtk-openjdk/blob/86c6f534ae57d03fa45e7a73b698f851c84ab943/mmtk/src/scanning.rs#L42

### Yes!  They are different!

We confirmed that each of the two methods is used in a different scenario.

-   `process_node` is only used by `trace_object`, and
-   `process_edge` is only used by `scan_object`.

And nothing calls both `process_node` and `process_edge` at the same time.

So let's split them into two traits.

But my colleague [Yi Lin] reminded me that there were other classes that extends
`TransitiveClosure` in the original MMTk in JikesRVM.  To be safe, I looked into
the original JikesRVM MMTk before making further decisions.

[Yi Lin]: https://qinsoon.com/

## Back in JikesRVM

Now we temporarily move away from Rust MMTk, and go back to JikesRVM MMTk.  If
the `TransitiveClosure` trait is designed like this in Rust, it must have its
roots back in JikesRVM.

### TransitiveClosure in JikesRVM MMTk

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
implementations of `processEdge` and `processNode`.  This allows its subclasses
to override one method while leaving the other failing.

<small>*(Note that the JikesRVM code was written before Java 8, so interface
methods could not have default implementations.)*</small>

[tc-jikes]: https://github.com/JikesRVM/JikesRVM/blob/0b6002e7d746a829d56c90acfc4bb5c560faf634/MMTk/src/org/mmtk/plan/TransitiveClosure.java#L29

### TransitiveClosure subclasses in JikesRVM MMTk

In JikesRVM, many classes inherit from `TransitiveClosure`.

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

In JikesRVM, `TraceLocal` is the counterpart of both `ProcessEdgesWork` and
`ObjectsClosure` of the current Rust MMTk.

-   `Space.traceObject` calls `TraceLocal.processNode` to enqueues newly visited
    objects,
    -   just like `XxxSpace::trace_object` calling
        `ProcessEdgesWork.process_node` in Rust MMTk.
-   `Scanning.scanObject` calls `TraceLocal.processEdge` to process each
    reference field (edge),
    -   just like `Scanning::scan_object` calling `ObjectsClosure.process_edge`
        in Rust MMTk.

In JikesRVM MMTk, each plan (GC algorithm) defines its own `TraceLocal`
subclass.

<small>*(Some GC algorithms. such as MarkCompact and Immix, even have more than
one `TraceLocal` for different kinds of traces.)*</small>

It looks like the `TransitiveClosure` is a proper interface for `TraceLocal`.
It implements both `processNode` and `processEdge`.

However, `TraceLocal` is the only class that implements both `processNode` and
`processEdge`.  Other classes don't.

### processEdge: Field visitors

Some subclasses of `TransitiveClosure` are related to reference counting (RC).
They are `RCZero`, `RCModifiedProcessor`, etc.  They only override the
`processEdge` method, assuming `processNode` is never called.

What do they do?

They are **field visitors**.  For example, `RCZero` visits edges and stores
`null` to each field.

```java
public final class RCZero extends TransitiveClosure {
    // Does not override processNode, leaving it failing

    public void processEdge(ObjectReference source, Address slot) {
        slot.store(ObjectReference.nullReference());
    }
}
```

During RC collection, `RCZero` is passed to `Scanning.scanObject` as the
callback to set all reference fields of an object to `null`.

```java
public abstract class RCBaseCollector extends StopTheWorldCollector {
    private final RCZero zero;  // This implements TransitiveClosure
    // ... more code omitted
    @Override
    public void collectionPhase(short phaseId, boolean primary) {
        // ... more code omitted
        if (phaseId == RCBase.PROCESS_DECBUFFER) {
            // ... more code omitted
            while (!(current = decBuffer.pop()).isNull()) {
                // ... more code omitted
                VM.scanning.scanObject(zero, current);  // Passing zero as callback
            }
        }
    }
}
```

Other RC-related classes are similar.  They record the fields or apply
decrements to the objects pointed by each field.

### processNode: to enqueue objects

The `TraceWriteBuffer` class only implements `processNode`.

<small>*(In fact, `TraceWriteBuffer` is the only class that overrides
`processNode` but note `processEdge` throughout the history of
JikesRVM.)*</small>

```java
public final class TraceWriteBuffer extends TransitiveClosure {
    private final WriteBuffer buffer;
    // ... more code omitted
    @Inline
    public void processNode(ObjectReference object) {
        buffer.insert(object.toAddress());
    }
}
```

`TraceWriteBuffer` is only used by `CMSMutator` (concurrent mark-sweep mutator).

```java
public class CMSMutator extends ConcurrentMutator {
    private final TraceWriteBuffer remset;
    // ... more code omitted
    @Override
    protected void checkAndEnqueueReference(ObjectReference ref) {
        if (ref.isNull()) return;
        if (barrierActive) {
            if (!ref.isNull()) {
                if      (Space.isInSpace(CMS.MARK_SWEEP, ref)) CMS.msSpace.traceObject(remset, ref);            // here
                else if (Space.isInSpace(CMS.IMMORTAL,   ref)) CMS.immortalSpace.traceObject(remset, ref);      // here
                else if (Space.isInSpace(CMS.LOS,        ref)) CMS.loSpace.traceObject(remset, ref);            // here
                else if (Space.isInSpace(CMS.NON_MOVING, ref)) CMS.nonMovingSpace.traceObject(remset, ref);     // here
                else if (Space.isInSpace(CMS.SMALL_CODE, ref)) CMS.smallCodeSpace.traceObject(remset, ref);     // here
                else if (Space.isInSpace(CMS.LARGE_CODE, ref)) CMS.largeCodeSpace.traceObject(remset, ref);     // here
            }
        }
        // ... more code omitted
    }
}
```

In simple terms, the code above means "Trace the object `ref` in the space it is
in, and, if it is the first time the object is traced, enqueue it in `remset`."

What's in common between `TraceWriteBuffer` and `TraceLocal` is that both of
them contain a buffer (a remember set, or the tracing queue) where the
`traceObject` method can enqueue newly visited objects to.  This is what
`processNode` is for, i.e. *enqueuing objects*.

### So why not introducing a dedicated interface?

There comes an interesting question:

>   *If some classes are just field visitors, why don't we have a dedicated
>   interface for it, and name it `FieldVisitor`?*

and

>   *If some classes are just places to enqueue objects, why don't we have a
>   dedicated interface for it, and name it `ObjectBuffer`?*

`TransitiveClosure` has been there for 15 years.  Many developers have made
contributions to MMTk, and some of them must have noticed the issues I talked
about.  Why have `TransitiveClosure` remained this way till today?

And why did we end up having this `TransitiveClosure` amalgamation in the first
place?

## Through the history

To answer these questions, I dug into the Git revision history of the JikesRVM
repository.

The `git blame` command can show me in which commit any line in any source file
is last modified.  I use [vim-fugitive], and it even allows me to follow a line
of code from one commit to another, and see every single change to a line of
code in history.

[vim-fugitive]: https://github.com/tpope/vim-fugitive

### Early days of object scanning

The history of `Scanning.scanObject` is the history of object scanning interface
and implementation.

[Back in 2003][jikesrvm-68e3-tree], MMTk (was JMTk back then) and JikesRVM were
more tightly coupled than they are today.  Unlike the modern `Scanning`
interface, the `ScanObject` class back then contained concrete implementations
directly. The [`ScanObject.scan`][jikesrvm-68e3-scan] method enumerates
reference fields, and directly calls the `Plan.traceObjectLocation` static
method, which does the load/traceObject/store sequence like our modern
`ProcessEdgesWork::process_edge` method.  Everything was hard-wired.  The
operation for visiting field was fixed.

[A commit in 2003][jikesrvm-e7bc-commit] introduced the
[`ScanObject.enumeratePointers`][jikesrvm-e7bc-enum] method which calls back to
`Plan.enumeratePointerLocation` which can be customised.  This allows a certain
degree of freedom of what to do with each field, instead of
load/traceObject/store.

[Another commit in 2003][jikesrvm-e719-commit] introduced the `Enumerate` class
which was subsequently renamed to `Enumerator` and [made fully
abstract][jikesrvm-c2ff-commit].

```java
abstract public class Enumerator implements Uninterruptible {
    abstract public void enumeratePointerLocation(Address location);
}
```

The `ScanObject.enumeratePointers` method then used `Enumerator` as the call
back instead calling into `Plan` directly, allowing the behavior of visiting
each edge to be fully customised.

As I conjectured, *some developers did notice that the call-back for
`scanObjects` should be customisable*, and `Enumerator` was introduced just for
that.

[jikesrvm-68e3-tree]: https://github.com/JikesRVM/JikesRVM/tree/68e36aac633743a7930516eb4c7536240152dc1e
[jikesrvm-68e3-scan]: https://github.com/JikesRVM/JikesRVM/blob/68e36aac633743a7930516eb4c7536240152dc1e/rvm/src/vm/memoryManagers/JMTk/vmInterface/ScanObject.java#L42
[jikesrvm-e7bc-commit]: https://github.com/JikesRVM/JikesRVM/commit/e7bc7a0b35b96d3182a8cb53f9548c38c90de579
[jikesrvm-e7bc-enum]: https://github.com/JikesRVM/JikesRVM/blob/e7bc7a0b35b96d3182a8cb53f9548c38c90de579/rvm/src/vm/memoryManagers/JMTk/vmInterface/ScanObject.java#L64
[jikesrvm-e719-commit]: https://github.com/JikesRVM/JikesRVM/commit/e7195af9696795f20acb4eb841bc9beec8e7d412
[jikesrvm-c2ff-commit]: https://github.com/JikesRVM/JikesRVM/commit/c2ff58e6a8499ad987ecb1c30a0f206f1036ef1c

### In 2006, just before that important change

Both the `Scanning.scanObject` and `Scanning.enumeratePointers` existed in [the
Scanning class][jikesrvm-beginning-scanning] before late 2006.

```java
public abstract class Scanning implements Constants, Uninterruptible {
    public abstract void scanObject(TraceLocal trace, ObjectReference object);
    public abstract void enumeratePointers(ObjectReference object, Enumerator e);
    // ... more code omitted
}
```

Both `scanObject` and `enumeratePointers` enumerate reference fields in an
object.  However, they are used in totally different places.

#### Scanning.scanObject

The `Scanning.scanObject` method was used for tracing, as it took `TraceLocal`
as parameter, and called `TraceLocal.traceObjectLocation` for each reference
filed.

Note that at that time, [TraceLocal was a root
class][jikesrvm-beginning-tracelocal]. There was no superclasses or interfaces
like `TransitiveClosure` for it to extend/implement. (`Constants` is an
all-static interface, and `Uninterruptible` is just a marker.)  This means
`scanObject` was only applicable to subclasses of `TraceLocal`, and nothing
else.

```java
public abstract class TraceLocal implements Constants, Uninterruptible { // no superclass
    public final void enqueue(ObjectReference object) throws InlinePragma { // traceObject calls this
        values.push(object);
    }
    
    public final void traceObjectLocation(Address objLoc, boolean root) // scanObject calls this
            throws InlinePragma {
        ObjectReference object = objLoc.loadObjectReference();
        ObjectReference newObject = traceObject(object, root);
        objLoc.store(newObject);
    }
    // ... more code omitted
}
```

#### Scanning.enumeratePointers

On the other hand, the `Scanning.enumeratePointers` could in theory be used by
any code that needs to enumerate reference fields.  At that time, it was used
for (deferred) reference counting.  The following was the "mark grey" operation
in trial-deletion for cycle collection in reference counting.

```java
public final class TrialDeletion extends CycleDetector
        implements Constants, Uninterruptible {
    private TDGreyEnumerator greyEnum;  // extends Enumerator
    // ... more code omitted
    private final boolean markGrey(ObjectReference object, long timeCap)
            throws InlinePragma {
        // ... more code omitted
        while (!object.isNull()) {
            // ... more code omitted
            if (!abort && !RefCountSpace.isGrey(object)) {
                RefCountSpace.makeGrey(object);
                Scan.enumeratePointers(object, greyEnum);  // pay attention to this call site
            }
            object = workQueue.pop();
        }
        return !abort;
    }
    // ... more code omitted
}
```

The call site of `Scan.enumeratePointers` passed a `TDGreyEnumerator` instance
which customised the behaviour of visiting fields.  It just forward the call to
`TrialDeletion.enumerateGrey`.

```java
public class TDGreyEnumerator extends Enumerator implements Uninterruptible {
    private TrialDeletion td;
    // ... more code omitted
    public void enumeratePointerLocation(Address objLoc) throws InlinePragma {
        td.enumerateGrey(objLoc.loadObjectReference());
    }
}
```

#### The obvious problem

We then had `Scanning.scanObject` for tracing, and `Scanning.enumeratePointers`
for RC.

But *why did we need both methods*?  `scanObject` was basically a special case
of `enumeratePointers` that called `TraceObject.enumeratePointerLocation`.

Could we **unify** them? Apparently someone noticed that, and he did a
refactoring.

[jikesrvm-beginning-scanning]: https://github.com/JikesRVM/JikesRVM/blob/600956237939e61b314535d485dfdfcbab2c0bbe/MMTk/src/org/mmtk/vm/Scanning.java
[jikesrvm-beginning-tracelocal]: https://github.com/JikesRVM/JikesRVM/blob/600956237939e61b314535d485dfdfcbab2c0bbe/MMTk/src/org/mmtk/plan/TraceLocal.java#L42

### Unifying scanObject and enumeratePointers with "raceStep"

In late 2006, [someone created a commit][jikesrvm-tracestep-commit] which
introduced a new version of reference counting collector, and at the same time
did "a huge refactoring" (see the commit message).

This commit created a class named `TraceStep`.  `Scanning.scanObject` then took
`TraceStep` as parameter instead of `TraceLocal`.  The `enumeratePointers`
method was removed.

```java
public abstract class TraceStep implements Constants, Uninterruptible {
    public abstract void traceObjectLocation(Address objLoc);
}

public abstract class Scanning implements Constants, Uninterruptible {
    public abstract void scanObject(TraceStep trace, ObjectReference object);
}
```

Obviously, `TraceStep` was intended to replaced `Enumerator` as the call-back
interface for `scanObject` to enumerating fields.  Both tracing and RC started
using `scanObject` from then on.

For tracing, the `TraceLocal` started to extend `TraceStep`.

```java

public abstract class TraceLocal extends TraceStep   // Now extends TraceStep
        implements Constants, Uninterruptible {
    public final void traceObjectLocation(Address objLoc) // called by scanObject
            throws InlinePragma {
        traceObjectLocation(objLoc, false);
    }
    public final void traceObjectLocation(Address objLoc, boolean root)
            throws InlinePragma {
        // ... just like before
    }
}
```

RC operations, such as "mark grey", started extending `TraceStep` instead of
`Enumerator`.

```java
public final class TrialDeletionGreyStep extends TraceStep  // Now extends TraceStep instead of Enumerator
        implements Uninterruptible {
    public void traceObjectLocation(Address objLoc) {
        ObjectReference object = objLoc.loadObjectReference();
        ((TrialDeletionCollector)CDCollector.current()).enumerateGrey(object);
    }
}

public final class TrialDeletionCollector extends CDCollector
        implements Uninterruptible, Constants {
    public TrialDeletionGreyStep greyStep;  // A TraceStep instead of Enumerator
    // ... more code omitted
    private final void markGrey(ObjectReference object) throws InlinePragma {
        // ... more code omitted
        Scan.scanObject(greyStep, object);  // now passes a TraceStep as arg
        // ... more code omitted
    }
}
```

This commit successfully unified the object scanning interface.
`Scanning.scanObject` became the only object-scanning method. It became
applicable to all `TraceStep` instances alike, and was no longer coupled with
`TraceLocal`. Good! Nicely done!

More over, this commit cleverly found a concept that describes both
`TraceLocal` and the various operations in reference counting, such as "mark
grey", "scan black", etc.  It was **"TraceStep"**.  Intuitively, all of them
were steps of tracing.

But really?  We will soon find that it is not really that clever.

[jikesrvm-tracestep-commit]: https://github.com/JikesRVM/JikesRVM/commit/64f538ca4d348f062f3afb313f519ffcbbbd22bd

### Here comes TransitiveClosure

In 2007, someone made [another commit][jikesrvm-2007] to "reorganise the core of
transitive closure", and the motivations were "concurrent collection" and
"implementing prefetching during trace".

In this commit, `TraceStep` was renamed to our familiar `TransitiveClosure`.

-   The `TraceStep.traceObjectLocation` method was renamed to
    `TransitiveClosure.processEdge`, and
-   a new method `processNode` was added.

```java
@Uninterruptible
public abstract class TransitiveClosure {
  /**
   * Trace an edge during GC.
   *
   * @param objLoc The location containing the object reference.
   */
  public void processEdge(Address objLoc) {
    VM.assertions.fail("processEdge not implemented.");
  }

  /**
   * Trace a node during GC.
   *
   * @param object The object to be processed.
   */
  public void processNode(ObjectReference object) {
    VM.assertions.fail("processNode not implemented.");
  }
}
```

Note that I didn't add any `// ... more code omitted` comment because back in
2007, that was [the entire class body of `TransitiveClosure`][jikesrvm-2007-tc].

`TraceLocal` now extends `TransitiveClosure` instead of `TraceStep`, and

-   the `traceObjectLocation` method was renamed to `processEdge`, and
-   the `enqueue` method was renamed to `processNode`.

```java
public abstract class TraceLocal extends TransitiveClosure {
    public final void processNode(ObjectReference object) {
        // ... like before
    }

    public final void processEdge(ObjectReference source, Address slot) {
        // ... like before
    }
    // ... more code omitted
}
```

A few days later, [a subsequent commit][jikesrvm-2257-commit] introduced a
`TraceWriteBuffer` class that extended `TransitiveClosure` and overrode
`processNode` alone, and not `processEdge`.

```java
public final class TraceWriteBuffer extends TransitiveClosure {
    // ... more code omitted
    @Inline
    public void processNode(ObjectReference object) {
        // .. as you have seen in previous sections.
    }
}
```

It remains the only class that overrides `processNode` but note `processEdge`
in the history of JikesRVM, even today.

With this change, `TransitiveClosure` started to serve two distinct purposes.

1.  the callback for `scanObject`, and
2.  the place to enqueue an object after tracing it.

If a class was only used in one of the two cases, it would override only one of
the two methods.

[jikesrvm-2007]: https://github.com/JikesRVM/JikesRVM/commit/f85c61257bbeda1efeed3a2f6a4ba5903cbe74e0
[jikesrvm-2007-tc]: https://github.com/JikesRVM/JikesRVM/blob/f85c61257bbeda1efeed3a2f6a4ba5903cbe74e0/MMTk/src/org/mmtk/plan/TransitiveClosure.java#L29
[jikesrvm-2257-commit]: https://github.com/JikesRVM/JikesRVM/commit/2257a7565c9920beb8455a6df0f8f6f8dbb2bae4

### 10 years passed...

`TransitiveClosure` remained this way in JikesRVM MMTk since then.

And the `TransitiveClosure` class grew a little bit.  [Some static fields and
methods about specialised scanning][jikesrvm-specscan] were added to it, as if
it were a good place to hold information for specialised scanning.

[jikesrvm-specscan]: https://github.com/JikesRVM/JikesRVM/blob/5072f19761115d987b6ee162f49a03522d36c697/MMTk/src/org/mmtk/plan/TransitiveClosure.java#L32

### ...and there was Rust MMTk.

In 2017, we started porting MMTk to Rust.

#### Porting MMTk to Rust

The `TransitiveClosure` was copied to the Rust version, except this time it
was represented as a Rust trait.

```rust
pub trait TransitiveClosure {
    fn process_edge(&mut self, slot: Address);
    fn process_node(&mut self, object: ObjectReference);
}
```

And `TraceLocal` was a trait that requires `TransitiveClosure`.

```rust
pub trait TraceLocal: TransitiveClosure {
    // ... other methods
}
```

The `Scanning.scan_object` method took `TransitiveClosure` as parameter, just
like JikesRVM MMTk.

```rust
pub trait Scanning<VM: VMBinding> {
    fn scan_object<T: TransitiveClosure>(
        trace: &mut T,
        object: ObjectReference,
        tls: OpaquePointer,
    );

    // ... more code omitted
}

```

And the `XxxSpace::trace_object` methods still took `TransitiveClosure` as
parameter, like this:

```rust
impl<VM: VMBinding> CopySpace<VM> {
    pub fn trace_object<T: TransitiveClosure>(
        &self,
        trace: &mut T,
        object: ObjectReference,
        allocator: Allocator,
        tls: OpaquePointer,
    ) -> ObjectReference {

    // ... more code omitted
}
```

And [there was still TraceLocal][mmtk-core-prewp-tl]. (Not any more now.)

```rust
pub trait TraceLocal: TransitiveClosure {
    // ... omitted.  There are many methods, but none is interesting here.
}
```

Like in JikesRVM MMTk,

-   the `scan_object` method still called `trace.process_edge` for each visited
    edge, and
-   the `trace_object` method still called `trace.process_node` to enqueue the
    object on first visit.

Initially, *we did not address the fact that TransitiveClosure served two
different purposes.* By that time, we had just begun porting MMTk to Rust.  **We
prioritised making Rust MMTk working**, and ported from JikesRVM MMTk in a style
closely resembled the original Java code.

And MMTk worked.  Not just worked, but worked for OpenJDK, JikesRVM and several
other VMs, too.

[mmtk-core-prewp-tl]: https://github.com/mmtk/mmtk-core/blob/96855d287fe5ea789a532f347d6ee37e6679c71f/src/plan/tracelocal.rs

#### Introducing work packets

We later removed `TraceLocal` and introduced the work packet system.

The work packet system represents each unit of work as a "packet" that can be
scheduled on any GC worker thread.  The `TraceLocal` class was replaced by two
work packets:

-   The `ProcessEdgesWork` work packet represents a list of edges to be traced.
-   The `ScanObjects` work packet represents a list of objects to be scanned.

A `ProcessEdgesWork` work packet also carries an object queue inside it.  It
needs the `process_node` method so that `XxxSpace.trace_object` can call it and
queue newly visited objects.

Therefore, `ProcessEdgesWork` implements TransitiveClosure because
`trace_object` expects it. And... remember?  That implementation startled me...

```rust
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

Then what visits edges when scanning an object?  It is now the `ObjectsClosure`
object.  It needs to provide `process_edge`, but `Scanning::scan_object` expects
`TransitiveClosure`.

So we have to do what the [...][wtf] we need to satisfy that requirement, as we
have seen before:

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

As you can see, **even after we migrated to the work packet system, and even
though we no longer have any type that overrides both `process_edge` and
`process_node`, we still kept both `process_edge` and `process_node` in the
`TransitiveClosure` trait**.

[rust-mmtk]: https://www.mmtk.io/

## Why do we end up having TransitiveClosure like this?

We have been startled by smelly code.  We have [expressed our anger, impatience,
surprise, etc., without explicit vulgarity][wtf].  We have looked into JikesRVM
for the old MMTk.  We have gone through the history to see the change of the
object scanning interface, and read the commits from developers with the
intention of improving MMTk.

But why do we end up having a `TransitiveClosure` trait like this?

### Have we noticed that object scanning is not necessarily part of tracing?

Yes.  The [`Enumerator`][jikesrvm-c2ff-commit] interface was introduced just for
that.  When called back from `scan_object`, it allows us to do anything to
reference fields.

### Have we refactored scan_object so it takes a simple callback instead of TraceLocal?

Yes.  When [`TraceStep`][jikesrvm-tracestep-commit] was introduced, We unified
`Scanning.scanObject` and `Scanning.enumeratePointers`.  `Scanning.scanObject`
was refactored to only depend on `TraceStep`, and `TraceStep` was an abstract
class with only an abstract method `traceObjectLocation(Address objLoc)`.

```java
public abstract class TraceStep implements Constants, Uninterruptible {
    public abstract void traceObjectLocation(Address objLoc);
}

public abstract class Scanning implements Constants, Uninterruptible {
    public abstract void scanObject(TraceStep trace, ObjectReference object);
}
```

This should have been the ideal interface for `Scanning.scan_object` for MMTk in
Java.  (Rust could use closure to make it more concise.)

### But why did we migrate away from it?

Probably only the author of [this commit][jikesrvm-2007] knows the exact reason.

To my understanding, I think it was because **`TraceStep` was such a good, but
a wrong, name**.

1.  We named it "TraceStep".

2.  We made it the superclass of `TraceLocal`.

3.  We then [introduced TraceWriteBuffer][jikesrvm-2257-commit].

4.  We noticed `TraceWriteBuffer` was another place to enqueue objects in
    addition to `TraceLocal`.

5.  Then we naturally thought that both `TraceWriteBuffer` and `TraceLocal`
    should have a common superclass that had a method named `enqueue`.

    But `TraceStep` doesn't have `enqueue`.

6.  Then we extended `TraceStep` into `TransitiveClosure`, and added `enqueue`.

    And we even renamed `enqueue` to `processNode` to make it consistent with
    `processEdge`.

7.  Then we have `TransitiveClosure` which had `processEdge` and `processNode`.

```java
public abstract class TraceLocal extends TransitiveClosure {
    public final void processNode(ObjectReference object) { ... }
}

public final class TraceWriteBuffer extends TransitiveClosure {
    public void processNode(ObjectReference object) { ... }
}
```

Wow!  "TransitiveClosure" was an even better name!  That was what `TraceLocal`
really was, i.e. computing the transitive closure of an object graph!  A
"transitive closure" is a graph!  A graph has nodes and edges!

```java
public abstract class TransitiveClosure {
  public void processEdge(Address objLoc) { ... }
  public void processNode(ObjectReference object) { ... }
}
```

But `TraceWriteBuffer` doesn't override `processEdge`, and `RCZero` doesn't
override `processNode`!

No worries.  We leave them "unreachable".

"Unreachable"?  That doesn't sound right.

But it worked... for 15 years.

The name "TransitiveClosure"  made so much sense that we stuck to
`TransitiveClosure` forever, even after we ported MMTk to Rust.

### But TraceStep is a wrong name.

1.  `TraceLocal`, "mark grey", "scan black", `RCZero` and so on are all steps in
    tracing,
2.  and those trace steps process edges,
3.  hence `Scanning.scanObject` should accept `TraceStep` as a call-back
    argument, so `Scanning.scanObject` can give `TraceStep` edges to process.

Wrong.

`Scanning.scanObject` accepts a callback argument not because the callback is a
step of tracing, but simply **because the callback visits edges**.

I don't really know what counts as a "trace step".

-   Does "assigning `null` to each reference field" count as a "trace step"?
-   Does "applying decrement operations to the reference counts of all neighbor
    objects" count as a "trace step" even if it is used in reference counting,
    only?
-   Is trial-deletion considered as a kind of tracing at all?

No matter what it is, isn't it much easier to just say

>   *"`Scanning.scanObject` accepts it as a callback argument because it visits
>   edges."*

### The nature of interfaces.

This is the nature of interfaces.  A reuseable component should not make
assumptions about its neighbours more than necessary.  This is **the
separation of concern**.

It is just like when we do the following in rust:

```rust
vec![1,2,3].iter().foreach(|e| {
    println!("{}", e);
});
```

The `Iterator::foreach` function accepts this closure not because it prints
things, but because it visits elements.  `foreach` is intended for visiting
elements.  The closure receives the object.  That is the contract of the
`foreach` method.  Whether it prints the element or how it prints the element is
not part of the contract.

So "Enumerator" was a right name.  It correctly describes the role of the object
in a `scanObject` invocation, that is, *"it enumerates fields"*, nothing more.
That's all what `Scanning.scanObject` need to care about.  It passes each field
to the callback, and that's it.  It should not assume what that callback object
does to each edge.  Whether it is a `TraceLocal` or an RC operation is beyond
its obligation.

"TraceStep" was wrong.  "TransitiveClosure" was also wrong.  Neither of them
is what `Scanning.scanObject` cares about.


## Finding the way out

We have seen the history, and know why it ended up like this.

We know what was wrong, and what would be right.

"Enumerator" was right.  "TraceStep" was wrong.  "TransitiveClosure" was wrong,
too, but it just sounded so good.

No matter how good it sounds, we need to fix it.

From our analysis in the beginning of this article, we should split
`TransitiveClosure` into two traits,

1.  one as the callback of `Scanning::scan_object`, and
2.  the other to be used by `XxxSpace::trace_object` to enqueue object.

I have opened an [issue][mmtk-core-issue-remove-tc], and detailed the steps of
splitting and removing `TransitiveClosure` from `mmtk-core`.

[mmtk-core-issue-remove-tc]: https://github.com/mmtk/mmtk-core/issues/559

### Refactoring Scanning::scan_object

`Scanning::scan_object` takes an object and a callback as parameters.  It will
find all reference fields in the object, and invoke the callback for each
reference field.

We need a proper name for the callback of `Scanning::scan_object`.

I name it `EdgeVisitor`, and its only method is, as you can imagine,
`visit_edge`.

```rust
pub trait EdgeVisitor {
    fn visit_edge(&mut self, edge: Address);
}
```

And it replaces `TransitiveClosure` as the parameter type of
`Scanning::scan_object`:

```rust
pub trait Scanning<VM: VMBinding> {
    fn scan_object<EV: EdgeVisitor>(
        tls: VMWorkerThread,
        object: ObjectReference,
        edge_visitor: &mut EV,
    );
}
```

Then the only callback that have ever been passed to `Scanning::scan_object`,
i.e. `ObjectsClosure`, now implements `EdgeVisitor`, instead.

```rust
impl<'a, E: ProcessEdgesWork> EdgeVisitor for ObjectsClosure<'a, E> {
    fn visit_edge(&mut self, slot: Address) {
        // ... code omitted
    }
}
```

And this change has been [merged][mmtk-core-commit-ev] into the master branch of
`mmtk-core`.

[mmtk-core-commit-ev]: https://github.com/mmtk/mmtk-core/commit/0babba20290d3c4e4cdb2a83284aa7204c9a23cc

#### Meanwhile in Australia...

While I was refactoring `mmtk-core` and working on `mmtk-ruby`, my colleague
[Wenyu Zhao][wenyu-homepage] was busy with [his paper about the LXC GC algorithm][paper-lxc] targetting [PLDI 2022][paper-lxc-pldi].

Wenyu [independently introduced][wenyu-commit-ei] the
[`EdgeIterator`][wenyu-file-ei] struct.  It is a wrapper over
`Scanning::scan_object` and `TransitiveClosure`, and the `unreachable!()`
statement, too! :P

```rust
pub struct EdgeIterator<'a, VM: VMBinding> {
    f: Box<dyn FnMut(Address) + 'a>,
    _p: PhantomData<VM>,
}

impl<'a, VM: VMBinding> EdgeIterator<'a, VM> {
    pub fn iterate(o: ObjectReference, f: impl FnMut(Address) + 'a) {
        let mut x = Self { f: box f, _p: PhantomData };
        <VM::VMScanning as Scanning<VM>>::scan_object(&mut x, o, VMWorkerThread(VMThread::UNINITIALIZED));
    }
}

impl<'a, VM: VMBinding> TransitiveClosure for EdgeIterator<'a, VM> {
    #[inline(always)]
    fn process_edge(&mut self, slot: Address) {
        (self.f)(slot);
    }
    fn process_node(&mut self, _object: ObjectReference) {
        unreachable!()
    }
}
```

With this struct, it allows a closure to be used in the place of a
`TransitiveClosure`.  The following code applies the `inc` RC operation to all
adjacent objects:

```rust
EdgeIterator::<E::VM>::iterate(src, |edge| {
    self.inc(unsafe { edge.load() });
})
```

And the following applies `dec`, and optionally frees the object:

```rust
EdgeIterator::<VM>::iterate(o, |edge| {
    let t = unsafe { edge.load::<ObjectReference>() };
    if !t.is_null() {
        if Ok(1) == super::rc::dec(t) {
            debug_assert!(super::rc::is_dead(t));
            f(t);
        }
    }
});
```

Pretty neat, isn't it?  It is so neat that I want to steal the code and add an
`EdgeVisitor::from_closure` factory method for my `EdgeVisitor`.

One interesting thing is, Wenyu introduced this for reference counting,
according to the [commit message][wenyu-commit-ei].  What a coincidence!  Daniel
[introduced `TraceStep` in 2006][jikesrvm-tracestep-commit] for exactly the same
reason: reference counting, according to its commit message, too.
Understandably, reference counting is very different from tracing.  RC needs to
scan objects, but not for tracing, so passing `TraceLocal` to `scan_object`
doesn't make sense.  Therefore, those additional operations, be it mark-grey,
scan-black or just freeing objects, all define their own callbacks to be called
by `scan_object`. This necessitates the creation of a better interface for the
callback of `scan_object`.

[wenyu-homepage]: https://wenyu.me/
[paper-lxc]: https://users.cecs.anu.edu.au/~steveb/pubs/papers/lxr-pldi-2022.pdf
[paper-lxc-pldi]: https://pldi22.sigplan.org/details/pldi-2022-pldi/15/Low-Latency-High-Throughput-Garbage-Collection
[wenyu-commit-ei]: https://github.com/wenyuzhao/mmtk-core/commit/9587aca2c62e02e043a5f01c3488cd91e21515b0
[wenyu-file-ei]: https://github.com/wenyuzhao/mmtk-core/blob/9587aca2c62e02e043a5f01c3488cd91e21515b0/src/plan/transitive_closure.rs#L88

### Refactoring Space::trace_object

The `XxxSpace::trace_object` method usually has this form:

```rust
impl<VM: VMBinding> CopySpace<VM> {
    #[inline]
    pub fn trace_object<T: TransitiveClosure>(
        &self,
        trace: &mut T,
        object: ObjectReference,
        semantics: Option<CopySemantics>,
        worker: &mut GCWorker<VM>,
    ) -> ObjectReference {
        // ... more code omitted
        if /* is first visited */ {
            let new_object = object_forwarding::forward_object::<VM>(...);

            trace.process_node(new_object);  // enqueue object
            new_object  // return the forwarded obj ref
        }
    }

    // ... more code omitted
}
```

If an object is first visited, it enqueues the object in `trace`, and returns
the forwarded object reference.

This method is polymorphic w.r.t. `T`.  `T` is the `ProcessEdgesWork` (a
sub-trait of `TransitiveClosure`) type used by the plan.  We used to have one
different `ProcessEdgesWork` type for each plan, which made this generic type
parameter necessary.

We already [noticed][mmtk-core-comment-remove-tc] that we may safely remove this
only use case of the `TransitiveClosure` trait (i.e. `trace_object`) once we
remove plan-specific `ProcessEdgesWork` implementations. And the good thing is,
we have recently just [removed][mmtk-core-commit-remove-pew] all plan-specific
`ProcessEdgesWork` implementations. Although we are not sure whether we will
have plan-specific `ProcessEdgesWork` for complex GC algorithms in the future, I
think the code is much cleaner for a refactoring.

However, I [believe][mmtk-core-issue-remove-tc] the `trace` parameter is
completely unnecessary.  We just need a return value to indicate whether it is
the first time the object is visited, so that the *caller* of `trace_object`
(which is only `ProcessEdgesWork::process_edge` at this time) can enqueue the
object.  So instead of

```rust
fn process_edge(&mut self, slot: Address) {
    let object = unsafe { slot.load::<ObjectReference>() };
    let new_object = self.trace_object(object);
    if Self::OVERWRITE_REFERENCE {
        unsafe { slot.store(new_object) };
    }
}
```

we shall have

```rust
fn process_edge(&mut self, slot: Address) {
    let object = unsafe { slot.load::<ObjectReference>() };
    let (new_object, first_visit) = self.trace_object(object);
    if Self::OVERWRITE_REFERENCE {
        unsafe { slot.store(new_object) };
    }
    if first_visit {
        self.enqueue(new_object);
    }
```

However, the `#[inline]` above `trace_object` indicates that it is very
performance-critical.  We'd better measure before making the decision to change.

[mmtk-core-comment-remove-tc]: https://github.com/mmtk/mmtk-core/issues/110#issuecomment-954335561
[mmtk-core-commit-remove-pew]: https://github.com/mmtk/mmtk-core/commit/93281e9563fb5a780b880c086f67c75fc66bc8f8

## Epilogue

After fifteen years, the `TransitiveClosure` trait is finally going to change.

The Rust MMTk is under active development now.  As we proceed, we may see more
things like this, things that have remained in its current state for years, or
even decades.  But this doesn't mean they are always right.  We have to rethink
about the code again and again, and fix the problems whenever we can.

## See also

The tracking issue: https://github.com/mmtk/mmtk-core/issues/559

<!--
vim: tw=80
-->
