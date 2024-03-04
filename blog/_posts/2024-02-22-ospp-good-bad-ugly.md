---
layout: post
title: "OSPP'2023: The good, the bad, and the ugly"
tags: mmtk,ospp
---

TL;DR: Last year, the MMTk project participated in the Open-Source Promotion
Plan (OSPP).  We mentored two students and they completed two student projects,
which was cheerful.  But the OSPP'2023 event itself was organised in a way I
found frustrating, and even hostile to the free software community.  Meanwhile,
I realised that there are toxic people lurking in the "open-source community" in
China, which is worrying.

# The Open-Source Promotion Plan (OSPP)

The United States started a trade war against China several years ago, and my
[former employer][Huawei] was on the 'Entity List'.  Depending on your political
stance, you may support one side or the other.  But the trade war raised a real
concern for every company about their software supply chain.  If your company
suddenly appears on the Entity List of some country (including but not limited
to the US), your favourite software may immediately become unavailable.  For
instance, [MATLAB] is no longer available to the [Harbin Institute of
Technology][HIT].

[Huawei]: https://huawei.com
[MATLAB]: https://www.mathworks.com/products/matlab.html
[HIT]: https://www.hit.edu.cn/

This is where [Free Software] comes into play.  What will you fear for if you
have the complete source code (and probably the entire revision history) in your
hard drive (not in the 'cloud'), and the contributors have given you a
perpetual, worldwide, non-exclusive, no-charge, royalty-free, irrevocable
license to run, copy, distribute, study, change and improve the software?  Free
Software (and hardware, too) is the ultimate answer to protectionism.  Since the
trade war started, the government and companies in China started to embrace free
software more than ever.  Some government agencies even switched to domestic
GNU/Linux distributions as their working environment for average workers.  Here
is a photo taken from the computer science section in a Xinhua Book Store in
Beijing.  The book store is currently under refurbishment, so only a small
fraction of books are displayed.  Even if you don't read Chinese, you can still
find books about open-source technologies such as Linux, Apache Pulsar, Kotlin,
Python, Java, OpenCV, Qt 6, Scratch, Go, R, OAuth2 and PostgreSQL.  This means
free software is no longer a niche technology, but a technology on popular
demand.

![Xinhua Bookstore]({% link /assets/img/xinhuashudian-shelf.jpg %})

[Free Software]: https://www.gnu.org/philosophy/free-sw.html

The [Open Source Promotion Plan (OSPP)][OSPP] is an event organised by the
[Institute of Software, China Academy of Science (ISCAS)][ISCAS].  According to
OSPP's official website, it is an explicit goal of OSPP to 'build the open
source software supply chain together'.  Similar to Google Summer of Code
(GSoC), OSPP funds students (both local and overseas) to participate in free
open-source software (FOSS) projects for three months (from July to September)
in Summer.  For students, such activities give them motivations for participate,
for the love of FOSS or just some extra cash.  For FOSS projects, such
activities give them an opportunity to attract contributors, and also get some
low-priority tasks done while main contributors are busy with high-priority
issues.  For the organiser, ISCAS, and the Chinese government backing it, this
event encourages more people to participate in FOSS development, which will
eventually yield a stronger free software community and help both Chinese users
and FOSS users worldwide to counter protectionism.  That is a win-win-win
situation.

[OSPP]: https://summer-ospp.ac.cn/
[ISCAS]: http://www.is.cas.cn/

The [MMTk] project participated in the OSPP in 2023, and I was the project
representative of MMTk.  Two students worked with MMTk, and I mentored one of
them.  The good thing is, both students finished their tasks, and we are
grateful to ISCAS for organising such an awesome event, and to the students for
their contributions.  However, the experience of participating in OSPP is not
100% pleasant, and I'll elaborate.

[MMTk]: https://www.mmtk.io/

# The good

I have always been a FOSS enthusiast.  In late 2000s and early 2010s, I attended
many off-line activities related to FOSS in Beijing (because I studied in
Beijing).  That included some student societies in my university, the [Software
Freedom Day][SFD], some local user group activities such as the [Beijing Linux
User Group][BLUG], the [Beijing GNOME User Group][BJGUG], etc.  Then I went to
Australia for my PhD, where I enjoyed exciting talks and pizzas with the
[Canberra Linux User Group][CLUG].  But I returned to China, I didn't find much
chance to get together with local communities due to work pressure.  I attended
one off-line event, but there were too few participants, probably due to
COVID19.  I missed the days when FOSS lovers (including both Chinese and people
from overseas) get together once a month, coding together, for fun.

[SFD]: https://www.softwarefreedomday.org/
[BLUG]: https://beijinglug.club/
[BJGUG]: https://www.bjgug.org/
[CLUG]: https://clug.org.au/

I heard about OSPP in late 2022, and thought MMTk should participate in the next
year in order to let more people know about MMTk, and probably let more people
know how awesome the Australian National University is, too.  I couldn't wait
telling FOSS lovers that we are doing interesting researches and working on an
open-source projects with collaborators from many different countries.  And we
did.  We applied for participation in OSPP'2023, and we were accepted.

In early 2023, I met an OSPP organiser in an off-line meeting.  She told me that
OSPP organisers had visited several universities in different cities in China in
order to get more students know about OSPP and free software in general.  I was
very excited to know that there are people working hard to promote free
software.  She also told me that although free software is already very popular
among top-tier universities in Beijing, it is far from true in lower-tier
universities in smaller cities.  I appreciated their efforts.

OSPP organisers invited participating communities to submit introduction videos
so that they can help promoting their projects by publishing the videos on
behalf of the communities.  Professor Steve Blackburn, the leader of the MMTk
project, gladly made a video to provide [a brief introduction to
MMTk][mmtk-video].  He even recorded it several times to correct minor mistakes.
I created Chinese subtitle for it and sent it to the OSPP organisers.

[mmtk-video]: https://youtu.be/0mldpiYW1X4

The event went on well.  We posted two student projects, one for [migrating our
JikesRVM binding to the new weak reference processing API][ospp-proj-weakref],
and another for [supporting ARMv8][ospp-proj-armv8].  Several students expressed
their interests in our [Zulip channel][mmtk-zulip-ospp], and some even asked for
easy issues for beginners to fix so that they could get familiar with MMTk.
Eventually, two students were selected by OSPP's matching algorithm, and I
mentored one of them to do the project related to JikesRVM and weak references.

[ospp-proj-weakref]: https://summer-ospp.ac.cn/org/prodetail/235730136
[ospp-proj-armv8]: https://summer-ospp.ac.cn/org/prodetail/235730272
[mmtk-zulip-ospp]: https://mmtk.zulipchat.com/#narrow/stream/265041-Summer-Projects/topic/OSPP

Mr. Haohang Shi, the student I mentored, demonstrated his ability to learn
things quickly.  I told him to look at our continuous integration (CI) scripts,
and told him that JikesRVM needs a 32-bit toolchain and an old version of JVM
(version 8), and I used LXC to make an isolate environment.  In just one week or
two, he managed to compile JikesRVM with the MMTk binding in a Docker container
in one command.  From then on, we had online meeting every week.  Things went on
mostly well, until Haohang got stuck with calling a Rust closure from JikesRVM.
The crux was passing the pointer to the closure object to and from the C code,
and we have similar code in our VM binding repos.  If this happens in our team,
i.e. if one of my colleagues has difficulty implementing something, I will just
write that part of the code for my colleague.  However, I didn't do this for
Haohang.  One reason was that the rules of OSPP forbid mentors from doing coding
for the students (obviously, otherwise some mentors would do everything for the
student and the student would just sit there and do nothing and get the bonus).
Another reason was that I didn't want to spoil all the fun of finding the
solution by oneself.  He eventually figured it out by himself after several
weeks.  That was a bit long, but he managed to finish the rest of the work
before the deadline and his code was eventually merged.

Meanwhile, good news came from the other student working on the ARMv8 port.  The
student was able to run DaCapo Benchmarks on an ARMv8 device with MMTk and
OpenJDK.  In the end, both student projects were successful.  We acknowledged
their contributions on [our website][mmtk-student-projects].

[mmtk-student-projects]: https://www.mmtk.io/projects

# The bad

While the student projects went on well, the organisation of the OSPP'2023 event
was far from perfect, and the experience as a participant is, to be honest, far
from satisfactory.

The [official website][OSPP] was poorly engineered.  It forced each user to have
exactly one role, making it very inconvenient for people who play several
different roles in OSPP'2023.  I had to log in with one email address as the
community representative, and another email address as the mentor.  Although
OSPP'2023 allowed communities to use Chinese or English or both, the website
didn't seem to be considerate enough for English users.  The registration forms
limit the length of some sections by the number of characters, making them
unfair for English which tend to have more characters than Chinese.  The
typesetting was also weird.  When breaking long lines, words can be broken
anywhere, disregarding any hyphenation rules of the English language.  The
following screenshot shows line-wrapping happening between 'Ope' and 'nJDK' in
the word 'OpenJDK', as well as many other unfortunate places.

![Screenshot with weird line wrapping]({% link /assets/img/ospp2023-linewrap.jpg %})

It ended up that someone set the CSS property `word-break: break-all;`.  I still
don't understand why anyone would do that, but I bet nobody tested the web page
with a proper English article and, if anyone actually did, the tester was not
aware of hyphenation rules.  Anyway, I reported the issue to the organisers and
they fixed it.

I mentioned that we made [an introduction video][mmtk-video].  Despite of the
efforts we made creating it, I didn't find the video published anywhere, and I
didn't get any reply saying whether our video was accepted or rejected.

The official method for communicating with the organisers was via email.  But
when I sent an email asking some questions, one organiser invited me to join a
WeChat group.  Despite its popularity in China, WeChat is a proprietary instant
messaging application mainly focusing on smart phone, and is terrible to use on
desktop computers.  I felt very strange because OSPP stood for 'Open Source
Promotion Plan'.  It was supposed to promote free open-source software.  Why do
they invite me to use a proprietary application?  But I joined the WeChat group
anyway to see what would follow.  That was a group with 'community
collaboration' in its title, and its members included 400+ representatives from
different communities.

After a few days, the organiser published a template pack for communities to
make their promotional 'posters', and here is a sample:

![Poster template]({% link /assets/img/ospp2023-poster.jpg %})

For those who don't know, those who use WeChat or other Chinese social media
such as Weibo for promotion usually make such one-picture posters and send them
in group-chat channels.  The advantage is obvious.  It is just so simple, and it
bypasses any text-formatting functionalities WeChat or other apps provided to
their users, which are often hard to use (if exist at all).  The QR codes are
usually URLs to social media accounts.

The problem is, the organiser provided the template pack using a link to [Baidu
Wangpan], a Chinese cloud storage service that requires a proprietary client to
use.  Seriously, was OSPP promoting open-source software by forcing participants
to use proprietary software?

[Baidu Wangpan]: https://pan.baidu.com/

After complaining to the organisers of OSPP'2023, they moved the poster
templates to [a GitHub repository][ospp-publicity].

[ospp-publicity]: https://github.com/summer-ospp/publicity

I browsed the repository.  They provided the posters in two formats: a `.sketch`
file and a `.figma` file.  The former was for [Sketch], a proprietary graphics
software exclusive to Mac; and the latter was for [Figma], a proprietary
web-based graphics software.  There were samples in `.pdf` and `.jpg`, too.
Are you kidding me?  Providing open-source promotion material in two proprietary
formats?

[Sketch]: https://www.sketch.com/
[Figma]: https://www.figma.com/

I complained to the organisers that there was absolutely no way for Linux users
to open the `.sketch` or `.figma` files, and I won't buy a Mac just for OSPP.
Even `.psd` could be slightly better because GIMP and Krita could open a subset
of `.psd` files.  An organiser replied in an embarrassed tone, saying that the
artist who designed that poster was just asking them whether they should publish
the `.psd` file, too.

From the conversation, it was obvious that the OSPP organisers hired an artist
to make a poster template for OSPP.  The artist was probably an average artist
who knew nothing about free open-source software, and the artist used their
familiar tool, which happened to be Adobe Photoshop, to do their job.  The
organisers of OSPP probably assumed that community representatives were artists
or other non-technical social media operators and probably used Mac, and
provided the templates in Mac-friendly formats.

**The organisers of OSPP were not organising an event for the free software
community, but an average event whose theme happened to be about free
software.**  When organising an event for the free software community, the
organisers need to understand what software FOSS contributors use.  There are
many free and open-source communication platforms, such as [Zulip] which MMTk is
using, as well as the good old IRC and mailing lists.  And there is free
graphics software, such as [GIMP] and [Krita] for bitmaps, and [Inkscape] for
vector graphics.  There are artists who are also free software lovers.  The
Krita community hosted [interviews][krita-interviews] with Krita as their main
creative tool.  If you need help from artists who love free software, you can
just ask, because **the free software community is all about creation and
sharing**.  The KDE community held a [Plasma 6 Wallpaper
Context][wallpaper-contest], with requirements including 'releasing under the
CC-BY-SA-4.0 license' and 'creating the wallpaper in a non-proprietary format'.
Many artists [submitted their work][wallpaper-submission], and the KDE community
eventually got a new default wallpaper for Plasma 6, as shown below:

![Plasma 6 Screenshot]({% link /assets/img/plasma6-desktop.png %})

[Zulip]: https://zulip.com/
[GIMP]: https://www.gimp.org/
[Krita]: https://krita.org/
[Inkscape]: https://inkscape.org/
[krita-interviews]: https://krita.org/en/categories/artist-interview/
[wallpaper-contest]: https://dot.kde.org/2023/08/14/calling-all-artists-plasma-6-wallpaper-contest
[wallpaper-submission]: https://discuss.kde.org/c/community/wallpaper-competition/26

It was sad because it had been three previous OSPP events and the organisers
were still yet to understand the nature of the free software community.

# The ugly




{% comment %}
vim: tw=80
{% endcomment %}
