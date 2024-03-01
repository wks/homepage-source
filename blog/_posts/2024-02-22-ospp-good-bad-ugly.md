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
GNU/Linux distributions as their working environment for average workers.

[Free Software]: https://www.gnu.org/philosophy/free-sw.html

The [Open Source Promotion Plan (OSPP)][OSPP] is an activity organised by the
[Institute of Software, China Academy of Science (ISCAS)][ISCAS].  According to
OSPP's official website, it is an explicit goal of OSPP to 'build the open
source software supply chain together'.  Similar to Google Summer of Code
(GSoC), OSPP funds students (both local and overseas) to participate in free
open-source software (FOSS) projects (also both local and overseas) for three
months (from July to September) in Summer.  For students, such activities give
them motivations for participate, for the love of FOSS or just some extra cash.
For FOSS projects, such activities give them an opportunity to attract
contributors, and also get some low-priority tasks done while main contributors
are busy with high-priority issues.  For the organiser, ISCAS, and the Chinese
government backing it, this activity encourages more people to participate in
FOSS development, which will eventually yield a stronger free software community
and help both Chinese users and FOSS users worldwide to counter protectionism.
That is a win-win-win situation.

[OSPP]: https://summer-ospp.ac.cn/
[ISCAS]: http://www.is.cas.cn/

The [MMTk] project participated in the OSPP in 2023, and I was the project
representative of MMTk.  Two students worked with MMTk, and I mentored one of
them.  The good thing is, both students finished their tasks, and we are
grateful to ISCAS for organising such an awesome activity, and to the students
for their contributions.  However, the experience of participating in OSPP is
not 100% pleasant, and I'll elaborate.

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
one off-line activity, but there were too few participants, probably due to
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

OSPP organisers asked participating communities to provide video introductions
so that they can publish them on behalf of the communities.  Professor Steve
Blackburn, the leader of the MMTk project, [made a video to provide a brief
introduction to MMTk](https://youtu.be/0mldpiYW1X4) in response.  I created
Chinese subtitle for it and sent it to the OSPP organisers.

The activity went on well.  We posted two student projects, one for [migrating
our JikesRVM binding to the new weak reference processing
API][ospp-proj-weakref], and another for [supporting ARMv8][ospp-proj-armv8].
Several students expressed their interests in our [Zulip
channel][mmtk-zulip-ospp], and some even asked for easy issues for beginners to
fix so that they could get familiar with MMTk.  Eventually, two students were
selected by OSPP's matching algorithm, and I mentored one of them to do the
project related to JikesRVM and weak references.

[ospp-proj-weakref]: https://summer-ospp.ac.cn/org/prodetail/235730136
[ospp-proj-armv8]: https://summer-ospp.ac.cn/org/prodetail/235730272
[mmtk-zulip-ospp]: https://mmtk.zulipchat.com/#narrow/stream/265041-Summer-Projects/topic/OSPP

Mr. Haohang Shi, the student I mentored, demonstrated his ability to learn
things quickly.  I told him to look at our continuous integration (CI) scripts,
and told him that JikesRVM needs a 32-bit toolchain and an old version of JVM
(version 8), and I used LXC to make an isolate environment.  In just one week or
two, he managed to compile JikesRVM with the MMTk binding in one command, using
Docker as a container.  From then on, we communicated via online video meeting
once a week, and occasional text messaging via Zulip.  Things went on mostly
well, until Haohang got stuck with how to call a Rust closure from JikesRVM.
The crux was the handling of the pointer to the context object, and we have
similar code inside our VM binding repos that do the same thing.  I would have
just written the code for him if this happened inside our MMTK team, i.e. if one
of my colleagues found something difficult to do when I happened to know the
answer.  However, I didn't do this.  One reason was that the rules of OSPP
forbid mentors from doing coding for the students (obviously, otherwise some
students would just sit there and do nothing and get the bonus).  Another reason
was that I didn't want to spoil all the fun of figuring out the solution by
oneself.  It took him several weeks to find the answer.  Fortunately, he
finished the rest of the work quickly after that and his code was eventually
merged.

Meanwhile, good news came from the student working on the ARMv8 port.  The
student was able to run DaCapo Benchmarks on an ARMv8 device with MMTk and
OpenJDK.  In the end, both student projects were successful.  We acknowledged
their contributions on [our website][mmtk-student-projects].

[mmtk-student-projects]: https://www.mmtk.io/projects

# The bad

While the student projects went on well, the organisation of the OSPP'2023
activity was far from perfect, and the experience of participating is, to be
honest, far from satisfactory.

The [official website][OSPP] was poorly engineered.  It forced each user to have
exactly one role, making it very inconvenient for people who play several roles
at the same time.  I am one of them, and I have to log in with one email address
as the community representative, and another email address as the mentor.
Although OSPP'2023 allows users, communities and projects to be registered in
either Chinese or English or both, the website didn't seem to be considerate
enough for English users.  The registration forms limit the length of some
sections by the number of characters, making them unfair for English which tend
to have more characters than Chinese.  The typesetting was also weird.  When
breaking long lines, words can be broken between any letters, disregarding any
hyphenation rules of English.  The following screenshot shows line-wrapping
happening between 'Ope' and 'nJDK' in the word 'OpenJDK'.

![Line-wrapping breaking 'OpenJDK' into 'Ope' and 'nJDK']({% link /assets/img/ospp2023-linewrap.jpg %})

It ended up that someone set the CSS property `word-break: break-all;`.  I
didn't know why anyone would do that, but I reported the issue to the organisers
and they fixed it.  I also wanted to spell my name in both Chinese characters
and Romanised form so that students who don't speak Chinese can read it, but the
website didn't allow that.





{% comment %}
vim: tw=80
{% endcomment %}
