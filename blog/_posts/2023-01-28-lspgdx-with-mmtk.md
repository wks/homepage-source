---
layout: post
title: MMTk in An Ancient Wuxia World
tags: mmtk l7p
---

TL;DR: During last Christmas, I tried to run [LSPGDX], a 3D FPS game implemented
in Java using OpenJDK with the MMTk binding.  It worked, but not perfectly.  GC
pauses are still a problem.

*Disclaimer: This is not part of the research of the [MMTk] project, and is not
sponsored by the Australian National University or Shopify.*

[MMTk]: https://www.mmtk.io/

# The first FPS game I played

When I was nine or ten years old, I played a PC game named
"摇滚少林系列之七侠五义3D", or "Rock'n'Shaolin: Legend of the Seven Paladins 3D"
(abbreviated as "L7P" or "LSP").  That was the first first-person shooting (FPS)
game I played.  That was an unusual FPS game because of its Chinese [Wuxia]
theme.  Instead of using guns, the player fights using ancient Chinese weapons
and "qigong", a powerful martial art that launches fireballs at the enemies.

[Wuxia]: https://en.wikipedia.org/wiki/Wuxia

![Title screen]({% link /assets/img/l7p-title.png %})
![L7P Game Play (melee)]({% link /assets/img/l7p-gameplay.png %})
![L7P Game Play (qigong)]({% link /assets/img/l7p-gameplay3.png %})

The game was released in 1990s and, like many games of that era, it ran on DOS.
However, many games of that era (such as Doom and Duke Nukem 3D) also released
the source code of their game engines so that developers could port the games to
modern platforms.  Such ports are called [source ports][source port].  For
example, [zdoom] is a port of Doom's "id Tech 1" engine, and [eduke32] is a port
of Duke3D's "Build" engine.  Those ports allow us to play those 1990s games on
modern GNU/Linux, MacOS, Windows, and many operating systems and hardwares you
can imagine.

[source port]: https://en.wikipedia.org/wiki/Source_port
[zdoom]: https://www.zdoom.org/index
[eduke32]: https://www.eduke32.com/

I wondered if there is a source port for the Legend of the Seven Paladins 3D,
too.  Fortunately, there is.

During Christmas last year, I found the [BuildGDX] project and [many other
projects developed by M210][m210-projects].  [BuildGDX] is a port of the Build
engine written in Java, and there are also ports of many Build engine games,
such as DukeGDX for Duke Nukem 3D, WangGDX for Shadow Warrior, BloodGDX for
Blood, and [LSPGDX] for L7P.  It ended up that L7P was also built on the Build
engine, and it was surprisingly the first game based on (an unreleased version
of) the Build engine!

[BuildGDX]: https://gitlab.com/m210/BuildGDX
[LSPGDX]: https://gitlab.com/m210/LSPGDX
[m210-projects]: https://m210.duke4.net/

I cloned the repositories, converted the Eclipse projects to Idea projects,
worked around some issues, and I managed to run it on my laptop, with ArchLinux
and OpenJDK 19.  Here is a screenshot.  Note that LSPGDX changed the HUD a
little bit to adapt to modern high-resolution displays.  The game window was a
bit small, though, because I hadn't figured out how to change the window size by
then.

![LSPGDX game play]({% link /assets/img/lspgdx-gameplay.png %})

# Can it run with MMTk?

Since it ran on OpenJDK, an immediate question came to my mind: "Does it run
with MMTk?"  My colleagues and I have been actively maintaining [mmtk-openjdk],
the OpenJDK VM binding of MMTk.  In version 10, OpenJDK refactored its GC
framework and introduced a GC interface, making it easy to plug in new GC
algorithms.  Our MMTk binding implements that interface and allows OpenJDK to
use any GC algorithms MMTk provides.  It should be a drop-in replacement for its
GC.

[mmtk-openjdk]: https://github.com/mmtk/mmtk-openjdk/

And it actually worked.  After fixing some [issues about soft
references][mmtk-openjdk-pr191], LSPGDX ran on OpenJDK 11 with the MMTk
binding.

[mmtk-openjdk-pr191]: https://github.com/mmtk/mmtk-openjdk/pull/191

![LSPGDX title screen with MMTk]({% link /assets/img/lspgdx-mmtk-1.png %})

Pay attention to the log output `[...  INFO mmtk::plan::global] User triggering
collection`. That was produced by MMTk core, and that meant the VM was actually
using MMTk.

I gave it a 128MB heap.  (That was way too generous.  Back in the DOS era, that
game ran with 4MB of total memory!)  The game manually triggers a GC during
start-up and another time when loading a saved game.

![LSPGDX game play with MMTk]({% link /assets/img/lspgdx-mmtk-2.png %})

As the player walked in the corridors, it only triggered GC once every half
minutes.  The game lagged a little bit when GC happens, but was hardly
noticeable.

However, when entering an area with a lot of enemies, like this one...

![Fighting many enemies]({% link /assets/img/lspgdx-fight.png %})

... it started to trigger GC once every several seconds.  What was worse, every
GC froze the game for about 3 seconds.  Note the timestamp of the log messages
"Triggering collection" and "End of GC" in the following screenshot.

![MMTk sometimes has very long GC pause]({% link /assets/img/lspgdx-mmtk-3.png %})

I stopped the game because the frequent GC pauses made the game unplayable.

The GC algorithm I chose was Immix, a high-throughput but non-generational
non-concurrent GC. There was a bug by then that prevented Generational Immix
from running.  But even if the GC was generational, once a full-heap GC
happened, it would just take as long as this one.

This experiment showed that GC latency matters for game applications.  And MMTk
does have a concurrent GC algorithm.  The concurrent [LXR] GC algorithm was
published last year, but has not been merged into the mainline MMTk core, yet.
I'll probably try playing LSPGDX with MMTk again when LXR stablises.

[LXR]: https://users.cecs.anu.edu.au/~steveb/pubs/papers/lxr-pldi-2022.pdf

# My forks

Since then, I have been hacking BuildGDX and LSPGDX to fix bugs and enhance the
gameplay.  If you are interested, you can clone my repositories on GitLab.

-   BuildGDX: <https://gitlab.com/wks/BuildGDX>
-   LSPGDX: <https://gitlab.com/wks/LSPGDX>

{% comment %}
vim: tw=80
{% endcomment %}
