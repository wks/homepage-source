---
layout: post
title: "Garbage Collection in Seoul"
tags: korea ismm2025 pldi2025 real-world-gc
excerpt_separator: <!--more-->
---

*Q: What's common between doing garbage collection in Seoul and doing garbage collection in CRuby?*

*A: Most objects need finalisation.*

I attended the [ISMM'2025] conference and presented [a paper][WBZ25-ismm].
I enjoyed the presentations and talked with interesting people.
What I found more interesting was how people do (real-world) garbage collection in Seoul, South Korea.

*WARNING: This article contains nerdy languages.*

<!--more-->

[ISMM'2025]: https://conf.researchr.org/home/ismm-2025
[WBZ25-local]: {% link downloads/pdf/ruby-ismm-2025.pdf %}
[WBZ25-acm]: https://dl.acm.org/doi/10.1145/3735950.3735960
[WBZ25-doi]: https://doi.org/10.1145/3735950.3735960
[WBZ25-ismm]: https://conf.researchr.org/details/ismm-2025/ismm-2025-papers/8/Reworking-Memory-Management-in-CRuby-A-Practitioner-Report

# The Paper

The paper I presented was [Reworking Memory Management in CRuby: A Practitioner Report][WBZ25-ismm].
It summarises our multi-year effort of integrating MMTk into CRuby, and it is full of challenges.
The paper is fully open-access.
You can download a copy [here][WBZ25-local] or [from the ACM digital library][WBZ25-acm] at no charge.
The organisers also published the video recording of my speech [on YouTube][talk-youtube].

[talk-youtube]: https://www.youtube.com/watch?v=-mTxhJJDWbw

# The Language

The conference was held in Seoul, South Korea.
This was the first time I visited a non-English-speaking foreign country.

The writing system was a challenge to me and my colleagues during the trip.
If I went to a country that uses Chinese characters (such as Japan), I (and my Chinese friends) could understand the Chinese characters even without knowing their pronunciations in the foreign language.
(But I actually learned Japanese before.)
If I went to a country that uses Latin-based alphabets (such as most European countries), I might be able to spot words that were spelled similar to English and guess their meanings.
But when I looked at Korean letters, I couldn't do anything to them.
I didn't know how they are pronounced, and I didn't know what they mean.

{% figure [caption:"The name of the road is written in four languages."] [class:"floating" %}
![Road names]({% link assets/img/seoul2025/road-names.jpeg %})
{% endfigure %}

Fortunately, as an internationalised city, I could find English text in many places, including road signs, corner shops, toilets (sometimes written as 'restroom' as in the United States), underground railways, and police stations.
Some roads even have official Chinese names on the road signs.
In busy business areas like Myeongdong, almost everything is written four languages: Korean, English, Japanese and Chinese.

Not all Korean people speak English well.
Staffs at big hotels and big shopping malls usually speak fluent English, and some of them can even speak Chinese.

Corner shop staffs and taxi drivers can speak basic English, but usually not that fluently.
In some traditional restaurants, the staffs often cannot speak other languages.
In such situations, we found translation software very handy.
Those modern AI-based apps allowed us to take a photo and translate the text on the picture.
Although none of my colleagues speak Korean, we managed to order authentic Korean food by translating the menu and communicating with the waiters with gesture languages.

{% figure [caption:"Translation software handles remote controls quite well."] [class:"floating" %}
![Bidet control and translation]({% link assets/img/seoul2025/bidet-control-and-translate.jpg %})
{% endfigure %}

And we sometimes find that we need translation service in unexpected places, such as the remote controls of air conditioners, the control panels of laundry machines, and the control panel of electronic bidets which are cute little devices that wash your private parts after using toilets.
Again the AI-based translation software made it quite convenient.

## Read the Spec

As a programmer, when I see a program in a programming language that I don't understand, I read the documentation, and sometimes its language specification if there is one.
I realised that I could do the same for human languages.
I searched for the documentation...

{% figure [caption:"A statue of Sejong the Great in Seoul"] [class:"floating"] %}
![Statue of King Sejong]({% link assets/img/seoul2025/king-sejong-statue.jpeg %})
{% endfigure %}

> In the ancient time, the Korean used the Chinese writing system.
> However, there is a semantic gap between the Korean and the Chinese languages.
> For this reason, average Korean people at that time couldn't express themselves efficiently in Chinese, just like [the Python programming language cannot be implemented efficiently on a high-performance JVM without language-specific optimisations][CEI12].
>
> With great sympathy to his subjects, the ancient King Sejong invented a new writing system optimised for the Korean language, with the explicit goal of making it easy to learn and convenient to use for everyone.
He wrote the language specification, a.k.a. [Hun Min Jeong Eum][] (literally 'Teaching of the Proper Sound for the People'), which not only specified the rules to pronounce and write the characters, but also introduced the rationale behind the visual design.

[CEI12]: https://doi.org/10.1145/2398857.2384631
[Hun Min Jeong Eum]: https://en.wikisource.org/wiki/Translation:Hunminjeongeum

King Sejong wrote [Hun Min Jeong Eum] in Classical Chinese (obviously he couldn't write a meta-circular spec due to bootstrapping issues), which made it easy for a Chinese like me.
The writing system was a genius invention.
Each letter is a diagram of tongue or mouth position, making it very easy to remember.
For example, 'ㄱ' resembles the shape of the tongue root closing the throat, giving it the /k/ sound; while 'ㄴ' resembles the shape of the tongue touching the hard palate, giving it the /n/ sound.
I spent an hour watching a few video tutorials about basic Korean pronunciation and vocabulary.
Although it still didn't enable me to make the most basic conversations, it allowed me to see a Korean character and know how it is pronounced.
Knowing the words '서울' (Seoul) and '우유' (milk) also allowed me to locate milk instead of soybean juice or alcohol when buying breakfast at corner shops.
(Don't mistake me.  I like soybean juice, too.)
And I noticed that some Korean words (such as '소화전' which means 'fire hydrant') are pronounced very similarly to their Chinese counterparts ('消火栓').

# Garbage Collection

In computer science, *garbage collection* refers to the memory management scheme that automatically recycles objects in the memory that cannot be accessed by the program.
In real world, *garbage collection* refers to a similar activity, i.e. recycling objects that we no longer need.

The bad thing is, nowadays we human beings are generating more garbage than ever.
We need to take action before our world is buried in garbage.

## Reduce, reuse and recycle

I like the slogan *reduce, reuse and recycle*.

Firstly, we should reduce the amount of garbage we generate, like how the [Lucene] indexing engine once changed its API and used *mutable* [Attributes][lucene-attribute] to reduce unnecessary allocations of objects.
When I was young, I carried my water bottle with me when I went to school.
But nowadays I usually buy bottled water from stores when I travel, especially on international trips where I couldn't take excessive amount of liquid past security.

[Lucene]: https://lucene.apache.org/
[lucene-attribute]: https://lucene.apache.org/core/10_2_2/core/org/apache/lucene/util/Attribute.html

Secondly, if I really have to buy bottled water from stores, I can still reuse the bottle by refilling it instead of throwing it away.
The apartment where I stayed was equipped with a water purifier, so I usually carried one bottle of purified water with me when I left for the conference, saving me one bottle of water (that is, 1000 Korean Won) every day.

Then we would need to recycle the remaining garbage we generated.
However, despite the technological advancement in the 21st century, humans are still required to manually sort garbage into categories before it can be recycled.
The garbage categorisation system varies from country to country.
In South Korea, they categorise recyclable garbage into glass, plastic, vinyl and (clean) paper.

## Finalisation

In computer science, *finalisation* refers to the actions to be taken when an object is determined to be dead.
It is a surprise that the similar actions are required in real-world garbage collection, too.

{% figure [caption:"Water bottle with instructions to remove the vinyl wrapper"] [class:"floating"] %}
![Garbage bins]({% link assets/img/seoul2025/bottle.jpeg %})
{% endfigure %}

Most garbage can be categorised straightforwardly, but there is a catch.
If a water bottle has a vinyl wrapper on it, it must be removed because it belongs to a different category.
As shown in the picture to the right, there are instructions on the water bottle that tells me to peel off the vinyl wrapper along the dotted lines.
Then I need to wash the bottle, and then dispose the vinyl wrapper into the 'vinyl' bin, and dispose the bottle itself into the 'plastic' bin.
Similarly, we are supposed to wash milk cartons and other containers.

As you can see, many objects need to be finalised before they can be recycled.
The experience of doing garbage collection in Seoul is similar to that in CRuby, albeit not that bad.
In CRuby, as we explained in Section 4.5 of [our paper][WBZ25-local], almost all objects need finalisation, just like doing GC in Seoul.

## In Public Areas

{% figure [caption:"Garbage bins near Myeongdong which have only two categories."] [class:"floating"] %}
![Garbage bins]({% link assets/img/seoul2025/garbage-bins.jpeg %})
{% endfigure %}

In public areas, such as in the street or at the airport, garbage bins usually come in only two categories, namely 'recycle' and 'waste'.
I suppose someone still needs to further categorise the garbage, but it makes the life easier for the tourists.

However, I felt there were fewer garbage bins in the streets of Seoul compared to other cities like Beijing and Canberra.
The organisers of the conference told us that Seoul removed many of the garbage bins in the city for security reasons.
Several decades ago, some bad guys put dangerous items into public garbage bins, which caused public security concerns.
Now garbage bags are all transparent, again for security reasons.

## At the Conference Venue

Considering that most foreigners are unfamiliar with the Korean garbage categorisation system, the conference organisers kindly arranged volunteers to patrol the conference ground and collect garbage, such as plastic bottles, that are left on tables. 
I found it whimsical to present a paper about garbage collection in a conference which was itself garbage-collected.

# Epilogue

The trip went on very well.
My peers and I presented our papers, and I met our collaborator [Peter Zhu] in person.
I also met some of my ex colleagues from [Huawei], and they had been making a very interesting programming language [Cangjie] and had extended the [LLVM] to better support garbage collection.
Unfortunately, I also found that some people, including professors from well-known universities, still having strong misconceptions about garbage collection and reference counting.
That's why we still need to work hard to make a garbage collection framework because not everyone knows how to do it right.

[Peter Zhu]: https://blog.peterzhu.ca/
[Huawei]: https://www.huawei.com/
[Cangjie]: https://cangjie-lang.cn/
[LLVM]: https://llvm.org/


{% comment %}
vim: tw=0
{% endcomment %}
