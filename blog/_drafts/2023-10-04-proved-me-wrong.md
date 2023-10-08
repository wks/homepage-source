---
layout: post
title: eBPF and Perfetto proved me wrong
tags: mmtk
---

This blog post is about an experience of debugging [MMTk] using an eBPF-based
tracing and visualisation tool.  During debugging, I tried to guess what went
wrong for many times, but the tool showed what actually happened and proved me
wrong each time.

[MMTk]: https://www.mmtk.io/

## The anomaly

I made [a pull request][pr794] for MMTk-core.  It sped up all benchmarks in the
DaCapo Benchmarks suite, except the `jython` benchmark which [became 3x
slower][pr794-3xslow] in stop-the-world (STW) time.  In the following plot,
`build3` is the baseline, `build1` is an intermediate commit, and `build2`
contains my final change.

[pr794]: https://github.com/mmtk/mmtk-core/pull/794
[pr794-3xslow]: https://github.com/mmtk/mmtk-core/pull/794#issuecomment-1521633617

![DaCapo results]({% link /assets/img/proved-me-wrong/pr794-stw.png %})

I could conclude that the `jython` benchmark had some abnormal behaviour and the
overall result (`build2` in `geomean`) is still speeding up.  But the 3x
slow-down is too significant to overlook.  I decided to investigate.

## eBPF-based tracing and visualisation

Strangely, the slow-down was only observable on two of the testing machines in
our laboratory.  When I re-ran the `jython` benchmark with build 1, 2 and 3 on
my laptop, their stop-the-world (STW) times were similar.  They were either
equally good or equally bad, but I did not know which is true.  I guessed they
were equally bad.  I inferred that the problem was non-deterministic.  It may be
triggered under some unknown conditions.

The slow-down was in the STW time (i.e. the time spent in doing GC).  How would
I know which part of the GC activities were slow?

At that time, my colleagues were developing a eBPF-based tracing tool which can
record the duration of each work packet during a GC, and visualise them on a
timeline.  Its overhead was so low that we can leave the trace points in the
code, enabled, in release builds.  I thought it was the perfect tool for my
task.

The tracing tool showed the timeline of what happened during a GC.  Actually, I
observed two different timeline patterns from different GCs when running the
`jython` benchmark.

The first pattern (I called it 'kind 1' GC) looked like this:

![timeline pattern of 'kind 1' GC]({% link /assets/img/proved-me-wrong/pr794-gc-kind1.png %})

This was a minor GC in a generational GC algorithm, and it was quite a typical
one.  The 'Prepare' stage happened before the green arrow at the 24.0792s mark,
where there were many work packets named `ScanXxxxRoot(s)`.  The green
`GenNurseryProcessEdges` work packets happen during the 'Closure' stage.  The
'Release' stage happens near the end of the GC where each worker executed some
clean-up job.  Since a minor GC does not trace much objects (it only traces the
nursery), the 'Closure' stage did not took much time.  A 'kind 1' GC typically
takes only about 1 ms, despite that one `GenNurseryProcessEdges` packet in Thread
192581 was a bit too big due to an unrelated load-imbalance problem.

The second pattern (I called it 'kind 2' GC) looked like this:

![timeline pattern of 'kind 2' GC]({% link /assets/img/proved-me-wrong/pr794-gc-kind2.png %})

This was also a minor (nursery) GC given the packet name
`GenNurseryProcessEdges`.  The prepare stage and the closure stage are similar
to the 'kind 1'.  (Please note the horizontal scale of the timeline.  The
prepare and the closure stages were in the far left of this plot, before the
`Finalization` packet.)  What was interesting was what were happening during the
`FinalRefClosure` stage where finalization were done.  After the `Finalization`
packet, lots of `GenNurseryProcessEdges` work packets were executed.  It was
obvious that finalization resurrected many objects.  A 'kind 2' GC may take more
than 15 ms to execute.

Given that finalization is non-deterministic, we might infer that the slow-down
was caused by an excessive amount of finalization due to non-deterministic
execution.  Problem solved..., or was it?  I still needed to verify.

## Printing out the GC times

I printed out the GC time of each GC in `jython` when running on a test machine
in the lab.

With 1500M heap size, the GC time (i.e. STW time) of the original code
(`build3`) and my modification (`build2`) were:

-   Before: 2,2,1,8,1,2,9ms
-   After: 1,21,1,10,1,20,9ms

When ran on another test machine with 1500M heap:

-   Before: 2,1,1,11,1,3,8ms
-   After: 2,27,1,9,2,25,8ms

So the results are similar.  I then tried to reduce the heap size.

When I reduce the heap size to 500M:

-   Before: 1,1,1,1,1,2,2,1,1,5,9,2,2,1,2,1,1,2,1,2,1,5,8,4
-   After: 1,1,1,1,1,30,1,1,1,4,8,1,1,1,1,1,1,27,1,1,1,3,8,4

When I reduce the heap size to 250M:

-   Before: 1,1,2,2,2,1,1,2,1,1,2,1,3,1,2,2,1,1,1,1,2,4,5,6,4,1,1,2,1,1,2,1,1,1,2,2,1,2,2,2,2,2,1,1,1,1,1,2,1,1,4,7,5,4,2,2,
-   After: 1,1,1,1,1,1,1,1,1,1,2,1,29,1,1,1,2,1,1,1,1,4,6,6,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,25,1,1,1,1,1,1,1,1,1,4,5,5,4,1,1,

If the slow-down were caused by excessive finalization, I would expect one build
to have more GCs that had more than 15 ms of STW time.  But it is not the case.
Two builds has the same number of GCs, and the GC time of each pair of
corresponding GCs were similar, too, except two pairs.  No matter how large the
heap size was, there were always exactly two GCs that took more than 20 ms in
the 'After' case (i.e. `build2`, the build with my PR applied).  For other GCs,
most of them took 1 ms and I assumed they were the 'kind 1' GCs; some of them
took 5 ms to 10 ms and I guess they were the 'kind 2' GCs.  They may run
slightly faster than my laptop.

So the two 20+ ms GCs were interesting.  What happened during them?  Were they
slow because of finalization, too?

I re-ran the test at 250M heap size on my laptop.  Strange things happened.

-   Before: 1,1,1,1,1,2,1,1,1,1,1,2,1,2,2,1,2,2,2,2,1,2,1,5,7,7,6,1,2,1,1,1,2,1,1,1,2,1,2,2,2,1,1,2,2,2,1,2,2,2,1,1,2,2,2,1,5,7,6,5,1,2,2,
-   After: 2,1,1,1,1,1,1,1,2,1,1,1,2,2,2,1,2,2,2,2,1,1,1,7,8,7,5,1,1,2,1,1,1,1,1,1,2,2,1,1,1,2,1,2,2,2,1,1,1,1,1,1,1,1,1,4,6,6,6,4,1,1,

The mysterious 20 ms GCs vanished!

Remember that I recorded the larger 'kind 2' GCs on my laptop.  This experiment
showed that the 20 ms GCs were not one of those 'kind 2' GCs at all, because
those 20 ms GCs never manifested on my laptop.

This also meant I could not capture those mysterious 20 ms GCs on my laptop
using eBPF tracing.  I had to do it on the lab machines.

## Manual tracing

Since I could not use eBPF tracing on the lab machine due to lack of root
access, I manually instrumented the code to let it print out trace logs at the
starts and ends of each work packets, processed them into the trace format
compatible to the result of eBPF, and fed the trace to [Perfetto
UI](https://www.ui.perfetto.dev/), the same visualisation tool used by my
colleagues' eBPF tool.  Then a nice timeline came out.  (And you can try it by
your self.  Load [this log file]({% link
/assets/data/proved-me-wrong/log_manual2.txt.json.gz %}) into
[https://www.ui.perfetto.dev/](https://www.ui.perfetto.dev/) and browse the
timeline interactively.)

![the 'after' build, manually traced]({% link /assets/img/proved-me-wrong/pr794-manual-perfetto-after.png %})

And that was a strange timeline, too.  Why was the `ProcessRegionModBuf` work
packet so large?  The purpose of the `ProcessRegionModBuf` work packet is
handling the write barriers for `System.arraycopy` operations on large arrays
between the last GC and this GC.

Why did the large `ProcessRegionModBuf` work packet appear in my execution?
There may be two possibilities:

1.  The `jython` benchmark did call `System.arraycopy` on a large array,
    therefore the work packet should exist in order to process the write
    barrier.  However, it did not appear in the 'Before' build (`build3`) due to
    a bug.
2.  The `jython` benchmark did not copy large arrays, but a bug (probably
    triggered by my PR) caused the write barrier to erroneously record an access
    to a large array, resulting in an abnormally large `ProcessRegionModBuf`
    work packet.

So which one is true?

## Hacking the write barriers

I added a log at the place where a `ProcessRegionModBuf` work packet is created
in order to see the sizes of the `ProcessRegionModBuf` work packets.

When running on the lab machines,

-   in the 'before' build, all `ProcessRegionModBuf` work packets had lengths of
    0;
-   in the 'after' build, `ProcessRegionModBuf` came in all different sizes.

When running on my laptop, all `ProcessRegionModBuf` work packets had lengths
of 0.

Then I had reasons to believe that it was not my PR that made the STW time
longer, but the STW time had been shorter than it should have been.  I believed
there was a bug in the write barrier code so that in the 'before' build, the
write barrier failed to record any accessed array regions, making the GC
erroneously faster than it should have been.  In the 'after' build, the bug was
not triggered, so the accessed array regions were recorded by write barriers as
usual, resulting in a longer but actually normal STW time.

One of my colleagues reminded me that there was an [un-merged
commit][commit-fix-wb] that fixes a bug in the write barrier code in the
MMTK-OpenJDK binding.  It was intended to fix an unrelated bug, but it was still
worth giving it a try.

[commit-fix-wb]: https://github.com/wenyuzhao/mmtk-openjdk/commit/1c384fd86ffe843afac779e85a98f6550a355923

I cherry-picked that commit, and...  Voila!  It fixed the bug.  The 'before'
build then exhibited the two 20+ ms GCs, just like the 'after' build.

-   Before: 1,2,2,1,1,29,1,1,2,5,8,2,1,1,1,2,1,29,1,1,1,4,9,3,

## Conclusion

At this point, we could draw the conclusion.  The STW time for the `jython`
benchmark should have been 3x longer.  However, due to the bug fixed by [this
commit][commit-fix-wb], the barrier didn't record the slices properly.  As a
result, when the heap is huge (such as 1500M) and GC is triggered only a few
times, the STW time for `jython` will be only 1/3 of the normal time.

However, my PR changed the behaviour in some way, and the bug was somehow not
triggered when running on the test machines in the lab.  And the sub-optimal
implementation of the `ProcessRegionModBuf` work packet stuffed all the slices
delivered from barriers into one single vector, making it impossible to
parallelise.  It gave us a false impression that my PR made the `jython`
benchmark 3x slower on the lab machines.

And due to non-determinism, the bug was still reproducible on my laptop with my
PR applied.  That was why I saw the 3x slow-down on the lab machines but not on
my laptop.

## What did I learn?

There is a saying that 'One can't optimise things that one can't measure'.  It
is very true.  I can hypothesise what caused the performance problem, but I can
never be sure unless I verify it with experiment.  In this example, I first
thought the slow-down was just some random behaviour of the `jython` benchmark.
I then thought the slow-down was due to excessive finalization.  But experiments
showed that the actual cause was a bug in the write barrier.

And the visualisation tool is a very useful one.  It first showed me that some
GCs took longer time due to finalisation, and it then showed me that there were
even longer GCs due to buggy and sub-optimal write barrier handling.  It is such
a useful tool because with it I no longer need to guess what slowed things down.
I just look at the timeline, and the work packet that becomes the bottleneck
will stand out of the crowd.  Since the eBPF-based tracing and visualisation
tool was created, I have been using it to debug many performance issues in GC,
including the execution of `obj_free`, various weak table processing tasks, and
the load-balancing problems in the [MMTk-Ruby binding][mmtk-ruby].

[mmtk-ruby]: https://github.com/mmtk/mmtk-ruby

For example, if you see the following timeline, you know the handling of
`obj_free` is the bottleneck.

![`obj_free` was the bottleneck in MMTk-Ruby]({% link /assets/img/proved-me-wrong/ruby-obj-free.png %})

If you see the following timeline (with manual annotation of which work packet
created which), you know the load balancing of the transitive closure stage
needs to be improved.

![bad load-balance during transitive closure]({% link /assets/img/proved-me-wrong/ruby-load-balance.png %})

And if you see the following timeline, you know the general load balancing is
improved, but some objects take significantly longer to scan than other objects,
and we should focus on those objects because they are the bottleneck.

![some objects took longer to scan]({% link /assets/img/proved-me-wrong/ruby-scan-object.png %})

I'd like to thank my colleagues for creating such a useful tool.  The paper that
describes this work has been accepted and will be published [in the MPLR'2023
conference](https://2023.splashcon.org/details/mplr-2023-papers/11/Improving-Garbage-Collection-Observability-with-Performance-Tracing).


<!-- vim: tw=80 -->
