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

## Should we split TransitiveClosure?

Maybe we should split it into *two different traits*, one contains
`process_node` and the other contains `process_edge`.  In this way, a type may
only implement the trait it needs, and not the `unreachable!()` stub.

Or, should we?

To confirm this, let's find their call sites, and see whether we ever use both
methods at the same time..

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
    -   just like `XxxSpace::trace_object` calling `ObjectClosure.process_edge`
        in Rust MMTk.
-   `Scanning.scanObject` calls `TraceLocal.processEdge` to process each
    reference field (edge),
    -   just like `Scanning::scan_object` calling
        `ProcessEdgesWork.process_edge` in Rust MMTk.

In JikesRVM MMTk, each plan (GC algorithm) defines its own `TraceLocal`
subclass.

<small>*(Some GC algorithms. such as MarkCompact and Immix, even have more than
one `TraceLocal` for different kinds of traces.)*</small>

It looks like the `TransitiveClosure` is a proper interface for `TraceLocal`.
Both `processNode` and `processEdge` are implemented.

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

>   *If some classes are just field visitors, why don't we have an interface
>   like `FieldVisitor`?*

and

>   *If some classes are just places to enqueue objects, why don't we have an
>   interface like `ObjectBuffer`?*

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
more tightly coupled.  Unlike the modern `Scanning` interface, the `ScanObject`
class back then contained concrete implementations directly. The
[`ScanObject.scan`][jikesrvm-68e3-scan] method enumerates reference field, and
called directly to the `Plan.traceObjectLocation` static method, which does the
load/traceObject/store sequence like our modern `ProcessEdgesWork::process_edge`
method.  Everything was hard-wired.  The operation for visiting field was fixed.

[A commit in 2003][jikesrvm-e7bc-commit] introduced the
[`ScanObject.enumeratePointers`][jikesrvm-e7bc-enum] method which calls back to
`Plan.enumeratePointerLocation` which can be customised.  This allows a certain
degree of freedom of what to do with each field, instead of
load/traceObject/store.

[Another commit in 2003][jikesrvm-e719-commit] introduced the `Enumerate` class,
and was subsequently renamed to `Enumerator` and [made
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
object.  However, they are used differently.

#### Scanning.scanObject

The `Scanning.scanObject` method was used for tracing, as it took `TraceLocal`
as parameter, and called `TraceLocal.traceObjectLocation` for each reference
filed.

Note that at that time, [TraceLocal was a root
class][jikesrvm-beginning-tracelocal]. There was no superclasses or interfaces
like `TransitiveClosure` for it to extend/implement. (`Constants` is an
all-static interface, and `Uninterruptible` is just a marker.)  This means
`scanObject` only applies to subclasses of `TraceLocal`.

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

### Unifying scanObject and enumeratePointers with "TraceStep"

In late 2006, [someone created a commit][jikesrvm-tracestep-commit] which
introduced a new version of reference counting collector, and at the same time
did "a huge refactoring".

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

What's more, this commit cleverly found a concept that describes both
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

-   the `traceObjectLocation` method was renamed to `processNode`, and
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

With this change, `TransitiveClosure` started to serve purposes.

1.  One was the callback for `scanObject`, and
2.  the other was the place to enqueue an object after tracing it.

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

And the signature of `CopySpace::trace_object` method was like this:

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

And [there was still TraceLocal][mmtk-core-prewp-tl].

```rust
pub trait TraceLocal: TransitiveClosure {
    // ... omitted.  There are many methods, but none is interesting here.
}
```

Like in JikesRVM MMTk,

-   the `scan_object` method called `trace.process_edge` for each visited edge,
    and
-   the `trace_object` method called `trace.process_node` to enqueue the object
    on first visit.

*We did not address the fact that TransitiveClosure served two different
purposes.* By that time, we had just begun porting MMTk to Rust.  We prioritised
making Rust MMTk working, and ported from JikesRVM MMTk in a style closely
resembled the original Java code.

And MMTk worked.

[mmtk-core-prewp-tl]: https://github.com/mmtk/mmtk-core/blob/96855d287fe5ea789a532f347d6ee37e6679c71f/src/plan/tracelocal.rs

#### Introducing work packets

We later removed `TraceLocal` and introduced the work packet system.

The work packet system presents each unit of work as a "packet" that can be
scheduled on any GC worker thread.  The `TraceLocal` class was replaced by two
work packets:

-   The `ProcessEdgesWork` work packet which represents a list of edges to be
    traced.
-   The `ScanObjects` work packet which represents a list of objects to be
    scanned.

A `ProcessEdgesWork` work packet also carries an object queue inside it.
`ProcessEdgesWork` loads form each edge and calls `XxxSpace.trace_object`.
Because `XxxSpace.trace_object` needs to enqueue the object when first visited,
`ProcessEdgesWork` implements the `TransitiveClosure` trait so that
`trace_object` can call `ProcessEdgesWork.process_node` to enqueue the object.
Then we see the code that startled in the beginning.

```rust
impl<T: ProcessEdgesWork> TransitiveClosure for T {
    fn process_edge(&mut self, _slot: Address) {    // Never called through the TransitiveClosure trait
        unreachable!();
    }
    #[inline]
    fn process_node(&mut self, object: ObjectReference) { // trace_object calls this
        ProcessEdgesWork::process_node(self, object);
    }
}
```

Although `ProcessEdgesWork` does have a `process_edge` method, it doesn't need
to implement it for `TransitiveClosure`, because `process_edge` is only called
internally, and is never called through the `TransitiveClosure` interface. The
only call site of `process_edge` through `TransitiveClosure` is still
`Scanning.scan_object`, as you have seen before.

As you can see, **even after we migrated to the work packet system, we still
kept both `process_edge` and `process_node` in the `TransitiveClosure` trait**.

[rust-mmtk]: https://www.mmtk.io/

## Why do we end up having TransitiveClosure like this?


## See also

See https://github.com/mmtk/mmtk-core/issues/559

We have identified this.  We mentioned that we could remove `TransitiveClosure`
in [this issue][mmtk-core-comment-remove-tc], and we just need someone to do it.

[mmtk-core-comment-remove-tc]: https://github.com/mmtk/mmtk-core/issues/110


<!--
vim: tw=80
-->
