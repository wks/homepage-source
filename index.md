---
layout: default
title: Home
---

<small> Psssssst! I have an unofficial [BitTorrent file][dacapo-torrent] for The
DaCapo Benchmark Suite 23.11-chopin.  Try it if the official download link is
too slow for you.</small>

{% include recent_blog_posts.html %}

## About Me

I am a programming language and virtual machine enthusiast.

I am a contributor to the [Memory Management Toolkit (MMTk)][mmtk] project. I am
actively developing an [MMTk binding for Ruby][ruby-mmtk] so that the
[Ruby][ruby] interpreter can use MMTk as its garbage collection backend.

I got my PhD degree from the [Australian National University][anu].  I designed
the [Mu micro virtual machine][mu].  You can find my publications here.

I worked for [Huawei][huawei] before.  The [Open Ark Compiler][openark] still
contains some of my code.  However, having participated in that project does not
mean I appreciate its design choices, especially [the use of naive reference
counting][openark-naiverc] as the garbage collection mechanism.

You can find my fun projects [on GitHub](https://github.com/wks).

I recently created [TUIModPlayer], a Rust program that plays [Module (a.k.a.
"mod")][mod] music in terminals.

[mmtk]: https://www.mmtk.io/
[ruby-mmtk]: https://github.com/mmtk/mmtk-ruby
[ruby]: https://www.ruby-lang.org/
[anu]: https://www.anu.edu.au/
[mu]: https://microvm.github.io/
[huawei]: https://www.huawei.com/
[openark]: https://gitee.com/openarkcompiler/OpenArkCompiler
[openark-naiverc]: https://gitee.com/openarkcompiler/OpenArkCompiler/blob/master/src/mrt/compiler-rt/include/collector/collector_naiverc.h
[TUIModPlayer]: https://github.com/wks/tuimodplayer
[mod]: https://modarchive.org/index.php?article-modules
[dacapo-torrent]: {% link downloads.md %}

## Publications

{% include publication_list.html %}

## Contact

Email: wks1986 AT gmail DOT com

{% comment %}
vim: tw=80 spell
{% endcomment %}
