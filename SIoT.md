# SIoT #
SIoT is a framework to analyze networked systems. SIoT’s
key insight is to look at a distributed system as a single body, and
not as separate programs that exchange messages. By doing so,
we can crosscheck information inferred from different nodes. This
crosschecking increases the precision of traditional static analyses.
To construct this global view of a distributed system we introduce a
novel algorithm that discovers inter-program links efficiently. Such
links lets us build a holistic view of the entire network, a knowledge
that we can thus forward to a traditional tool.


For more information on what SIoT is and what it can do, see our [IPSN 2015 paper](http://dl.acm.org/citation.cfm?id=2737097).

SIoT was implemented
on top of [LLVM](http://llvm.org/).
Access the SIoT [code](https://code.google.com/p/ecosoc/source/checkout?repo=siot), see the [README](https://code.google.com/p/ecosoc/source/browse/README?repo=siot) to getting start and enjoy it!