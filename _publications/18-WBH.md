---
key:    WBH18
kind:   inproceedings
author: Kunshan Wang, Stephen M Blackburn, Antony L Hosking and Michael Norrish
title:  "Hop, Skip, & Jump: Practical On-Stack Replacement for a Cross-Platform Language-Neutral VM"
in:     "14th ACM SIGPLAN/SIGOPS International Conference on Virtual Execution Environments (VEE 2018), March 24–25, 2018, Williamsburg, VA, USA."
year:   2018
pdf:    "osr-vee-2018.pdf"
doi:    "10.1145/3296975.3186412"
link:   https://dl.acm.org/doi/10.1145/3296975.3186412
---

On-stack replacement (OSR) is a performance-critical technology for many
languages, especially dynamic languages. Conventional wisdom, apparent in
JavaScript engines such as V8 and SpiderMonkey, is that OSR must be implemented
in a low-level (i.e., in assembly) and language-specific way.

This paper presents an OSR abstraction based on Swapstack, materialized as the
API for a low-level virtual machine, and shows how the abstraction of
resumption protocols facilitates an elegant implementation of this API on real
hardware. Using an experimental JavaScript implementation, we demonstrate that
this API enables the language implementation to perform OSR without the need to
deal with machine-level details. We also show that the API itself is
implementable on concrete hardware. This work helps crystallize OSR
abstractions and, by providing a reusable implementation, brings OSR within
reach for more language implementers.
