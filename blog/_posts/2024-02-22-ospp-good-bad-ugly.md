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
they invite me to use a proprietary application?

But I joined the WeChat group anyway to see what would follow.  That was a group
with 'community collaboration' in its title, and its members included 400+
representatives from different communities.  People were asking questions and
pointing out issues about the event, while the organisers replied.

After a few days, the organiser published a template pack for communities to
make their promotional 'posters', and here is a sample:

![Poster template]({% link /assets/img/ospp2023-poster.jpg %})

For those who don't know, those who use WeChat or other Chinese social media
such as Weibo for promotion usually make such one-picture posters and publish
them as tweets, or send them to group-chat channels.  The advantage is obvious.
It is just so simple, and it bypasses any text-formatting functionalities WeChat
or other instant message / social media apps provides to their users, which
are often hard to use (if exist at all).

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
Organising a free software event this way sent a very bad message to the public
that they didn't really care about software freedom.  Ironically, [OpenEuler], a
domestic Linux distribution, was a co-organiser of OSPP'2023.  Using free image
formats would have helped OpenEuler demonstrate that their operating system was
suitable for everyday use, but they didn't.  I couldn't help visualising an
OpenEuler user going nuts when they couldn't use their own operating system to
open an image file from their own event.

[OpenEuler]: https://www.openeuler.org/

In the end, we didn't use their poster templates because a Markdown-formatted
post in our Zulip channel was just enough for that purpose, and we didn't have a
social media account.  I'd rather spend one afternoon fixing more bugs than
trying to edit PDF or JPEG files directly, which would result in bad-looking
images anyway.  I raised an [issue][ospp2023-issue], asking the organisers to
provide materials in free formats.  However, at the time of writing, nobody had
replied to that issue.

[ospp2023-issue]: https://github.com/summer-ospp/publicity/issues/1

# The ugly

Although the organisers of OSPP'2023 didn't do a perfect job organising the
event, they promised to do it better next time when I criticised them in the
WeChat channel.  I think there is still hope.  Given enough time, OSPP will
eventually be organised in the right way.

However, there was still one thing that could be worse --- the communities.  Not
all communities, of course.  Only a few of them.

An interesting fact was, although students were required to be enrolled in
universities, there was no such requirement for communities (any communities
using OSI-approved open-source licenses would qualify).  No offend to the people
who didn't go to university and still made excellent free software, but that
rule meant that you had to expect participating communities of any quality, and
their members to be any kind of people you may possibly meet in the street.

When I was invited into the WeChat group of 400+ community representatives, I
found some members were greeting each other and offering to give stars to each
other's GitHub repositories.  One of them 'kindly' asked if he could give a star
to MMTk's GitHub repository.  I didn't thank him, but I replied that he needed a
five-star rating system for GitHub so that he could give one star to every repo
he barely knew, and five stars to the repos he honestly loved.  I consider it
cheating to blindly give stars to repositories of one's 'friends' because that
would make stars meaningless.  A repo with lots of stars may mean many people
genuinely like it, and it may also mean the author has many 'friends'.  I still
prefer that stars mean the former.

While community representatives were discussing about the event in the WeChat
group, several communities spammed the channel with news about every single
point release of their software.  Those posts became dominant since July when
the programming phase started and very few questions were asked about
administrivia.  Posts from each of the spammers look horribly similar, as if
they were sent by bots.  That was annoying.  I thought this WeChat channel was
for communicating with OSPP organisers.

Well, since they dared spamming, I dared challenging them.

I looked at one project.  It advertised as a web framework, with [Spring]-like
IoC container and annotation-based URL routing, but also supported GraalVM and
claimed to be many times faster than Spring.  Web, IoC and Graal.  Didn't that
sound familiar?  Yes.  [Micronaut].  That is a Web framework, with Spring-like
IoC container, annotation-based URL routing, and GraalVM support, too.  I heard
about Micronaut about four or five years ago, but this project seemed to be
recently started, and poorly documented.  I asked its representative how his
project was different from Micronaut.  'The author has never heard of
Micronaut', he answered, embarrassed, 'If I have to find any difference, it's
that our project was made in China.' Their official web site compared their
framework against Spring all the time and claimed to be much faster, but never
mentioned Micronaut at all.  That was not good.  You simply can't claim your
project is fast without comparing against the main contenders.

[Spring]: https://spring.io/
[Micronaut]: https://micronaut.io/

Then another project spammed, and I looked at that, too.  It was a simple socket
abstraction layer written in Java, similar to [Netty], but poorly documented,
and claimed to be twice as fast as Netty.  I looked at the code for a moment and
found something like this:

[Netty]: https://netty.io/

```java
} catch (Exception e) {
    e.printStackTrace();
}
```

And in another function, we could find something like this:

```java
try {
    accept(...);
} catch (Throwable e) {
    setState(...);
    accept(...); // Errors not handled
} finally {
    makeSession(...);
}
```

That didn't look very professional, did it?  Anyone who has learned Java for a
few months knows that `e.printStackTrace()` is not the right way to handle
errors, and retrying a failed operation doesn't guarantee that it will be
successful.  Omitting all the error-handling code and running micro benchmarks
that always succeed will always give you good numbers, but that's meaningless.
I told its representative that they shouldn't claim they were faster than Netty
while they were not doing exception handling properly, just like we can't claim
a GC algorithm was faster while the write barrier was deliberately turned off.
Otherwise people would challenge the result.  'Our project has been challenged
all along', replied the project representative, 'and I have long been used to
it.  Only those who used our projects know how pleasant it is.'  Yuck!  What an
irresponsible developer!

But, strangely, other people in the WeChat group started to speak for him.  One
person said, 'Some people want security while others want convenience.  Everyone
will get what they love.' Another person said, 'As a pawn \[of a company\], I
can't care less about how users feel.  Only the CTO needs to care about users,
and we programmers only need to please ourselves.'  Seriously?  I asked whether
they care about downstream projects at all, and apparently they didn't.  Knowing
this, I stopped arguing because that would be futile.

After I complained about OSPP'2023 not providing poster templates in free
formats, the organisers said they will be more considerate the next time.
However, other users in the WeChat group started whining.  Some said in some
fields, free software were far worse than their proprietary counterparts,
therefore we (OSPP) may use proprietary software.  Some said it was good to
practice the spirits of free software in this event, but we don't need to force
it.  Others said 'OSPP gives you money and you should shut up and do what you
are asked to'.  But let's look at the poster sample again.

![Poster template]({% link /assets/img/ospp2023-poster.jpg %})

Does it look good?  It does.  Well, it is good enough as a poster for an event
like OSPP, but not *that* good.  It is not so good that it has to be drawn using
best-of-the-breed tools like Adobe Photoshop.  Can someone create a poster of
similar quality (or even better) using only free software?  Given the result of
the [wallpaper contest][wallpaper-contest], the answer is obviously yes.  So
this is not a question of why using free software.  It's why not.  OSPP was a
very good opportunity to put software freedom into practice, and it was not hard
to do.  Why do we even have to 'force' using free software?

And when I shared the [interviews with Krita artists][krita-interviews], someone
said he could not understand those interviews because he doesn't speak English
well.  Seriously?  Chinese schools start teaching English since the third year
in school.  By the time when students finish high school, they will have been
learning English for ten years.  How could anyone learn English for such a long
time and still can't understand simple interviews like those?

And someone mentioned me in the group chat, telling me he bought a Mac, a very
expensive model, for opening proprietary formats.  I started to understand that
someone already started to hate me since I complained about all those
proprietary formats and irresponsible developers.  I told him that he could have
donated the money to support three students.  He apparently broke down mentally,
and started telling a pathetic story about himself, including quitting high
school, joining a open-source organisation, making 5000+ commits per year (that
is about 20 commits per working day, or about 20 minutes per commit) which most
people don't believe.  Strangely, several other people started to sympathise
with him.  One said he didn't have a high degree and had a hard time finding a
job.  Another said he only got 28 points (out of 150) in an English exam, and he
could have gone to Peking University if he got 100 points or more.  Come on.
Failing an exam is not something to be proud of.  

How weird!  I couldn't imagine what kind of people are there in the community.

Later that year, a friend of mine visited me in Beijing after living overseas
for many years.  He told me that many companies in China were simply copying the
business model of companies overseas, but most of them had no idea what they
were doing, and will bankrupt quickly.  That meant if anyone or any company was
doing honest research and development work, they will already be better than 90%
of their peers in China.  

Wait!  That sounded horribly familiar!  I told my friend about the open source
project so similar to Micronaut.  I also mentioned a recent news about CEC-IDE
([see this][cec-ide-news]).  CEC-IDE claimed to be an IDE fully original and
fully made-in-China.  But it ended up that they took the source code of [Visual
Studio Code][vscode], changed its name, added a login window for paid 'VIP'
services, added some plug-ins, and labelled it as a home-brewed software (and it
was close-sourced), an 'independently software developed by a state-owned
corporation, with trustworthy quality'.  Ironically, CEC-IDE was on the CCTV
(China Central Television) News when it was released, not long before it became
the laughing stock of all Chinese netizens.

[cec-ide-news]: https://www.pixelstech.net/article/1693110126-Chinese-Developers-Release-CEC-IDE-Claimed-as-First-Independently-Developed-IDE
[vscode]: https://code.visualstudio.com/

And everything suddenly became clear.  Here is the whole story:

1.  Since the trade war started, both the government and companies worried about
    their software supply chains.
2.  They started pouring money into the field of domestic software and
    open-source software, thinking they could be the rescue.
3.  Then some developers saw this as an opportunity.  Since the consumers wanted
    domestic and open-source software, they make domestic and open-source
    software for them.
4.  But it is difficult to develop software from scratch.  Instead, they simply
    copied existing open-source projects and label them as domestically
    developed.

Some copycats simply took the source code and changed the name.  Other more
sophisticated copycats wrote code from scratch, using existing open-source
projects as frames of reference.  Of course they didn't care about the user, the
documentation or the code quality.  As long as it was made-in-China and
open-source, investors would throw money into it, and average people would hail
it as the future star of China when they saw it on the news.

And this is why when you open any project on the OSPP'2023 home page, chance is
high that the project will be labelled as 'domestically developed, with
self-owned intellectual property'.  

But at what cost?

A 'software supply chain' built this way will be brittle, if working at all.
High-level applications will be built on flawed low-level libraries which don't
even handle exceptions properly and may break at any time.  But will the
developers care?  Probably not.  They would worry about their house rent and
mortgage much more than software quality.  Their bosses wouldn't worry about
software quality, either, if their companies monopolise a field, for example,
food ordering.  When the end users have to choose between one faulty software
and another, all they can do is pressing the reload button and pray it would
work the next time.

And will we win the trade war this way?  Well, I won't call it a win if average
people are oppressed by faulty software instead of foreign countries.

# Summary

OSPP'2023 was awesome.

Students were smart and passionate.

The organisers tried hard keeping the event running, but there were much room
for improvement.

The communities?  Most of them were honestly doing free software developments,
while some people are completely jerks.

So should I tell people the Australian National University is awesome, and our
research group is doing interesting researches?  Yes, to the students.  Some
students already expressed their interests in studying in the ANU when I told
them about it.  As of those who can't even read simple interviews in English,
'They were not our target audience', said one of my colleagues.

{% comment %}
vim: tw=80
{% endcomment %}
