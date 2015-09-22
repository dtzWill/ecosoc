# Flow Tracker #

Flow tracker is a tool that detects [timing attack](http://en.wikipedia.org/wiki/Timing_attack) vulnerabilities in C/C++ programs that implement cryptographic routines. If the program contains a vulnerability, then Flow Tracker finds one or more (static) traces of instructions that uncover it. A vulnerable trace describes a sequence of data and control dependences from a secret information - e.g., a crypto key, the seed for a random number generator, etc - to a predicate of a branch. This is a problem because the branch normally influences the execution time of the program. So, if an adversary can measure this time for a given input which he controls, it may discover some information about the secret that we want to protect.

THIS CONTENT HAVE BEEN MOVED TO http://cuda.dcc.ufmg.br/flowtrackervsferrante/install.html

# Contact #

If you have questions, you can submit it to brunors@dcc.ufmg.br or brunors172@gmail.com