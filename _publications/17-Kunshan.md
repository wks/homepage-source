---
key:      Wang2017
kind:     phdthesis
author:   Kunshan Wang
title:    "Micro Virtual Machines: A Solid Foundation for Managed Language Implementation"
institution: College of Engineering and Computer Science, The Australian National University
year:     2017
pdf:      "wang-thesis-2018.pdf"
---

Today new programming languages proliferate, but many of them suffer from poor
performance and inscrutable semantics.  We assert that the root of many of the
performance and semantic problems of today's languages is that language
implementation is extremely difficult.  This thesis the fundamental challenges
of efficiently developing high-level managed languages.

Modern high-level languages provide abstractions over execution, memory
management and concurrency. It requires enormous intellectual capability and
engineering effort to properly manage these concerns.  Lacking such resources,
developers usually choose naive implementation approaches in the early stages
of language design, a strategy which too often has long-term consequences,
hindering the future development of the language.  Existing language
development platforms have failed to provide the right level of abstraction,
and forced implementers to reinvent low-level mechanisms in order to obtain
performance.

My thesis is that the introduction of micro virtual machines will allow the
development of higher-quality, high-performance managed languages.

The first contribution of this thesis is the design of Mu, with the
specification of Mu as the main outcome.  Mu is the first micro virtual
machine, a robust, performant, and light-weight abstraction over just three
concerns: execution, concurrency and garbage collection.  Such a foundation
attacks three of the most fundamental and challenging issues that face existing
language designs and implementations, leaving the language implementers free to
focus on the higher levels of their language design.

The second contribution is an in-depth analysis of on-stack replacement and its
efficient implementation.  This low-level mechanism underpins run-time
feedback-directed optimisation, which is key to the efficient implementation of
dynamic languages.

The third contribution is demonstrating the viability of Mu through RPython,
a real-world non-trivial language implementation.  We also did some preliminary
research of GHC as a Mu client.

We have created the Mu specification and its reference implementation, both
of which are open-source.  We show that that Mu's on-stack replacement API can
gracefully support dynamic languages such as JavaScript, and it is
implementable on concrete hardware.  Our RPython client has been able to
translate and execute non-trivial RPython programs, and can run the RPySOM
interpreter and the core of the PyPy interpreter.

With micro virtual machines providing a low-level substrate, language
developers now have the option to build their next language on a micro virtual
machine.  We believe that the quality of programming languages will be improved
as a result.

{% if false %}
vim: tw=72 spell
{% endif %}
