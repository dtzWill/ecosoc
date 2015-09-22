## E-CoSoC: Energy-Eﬃcient Code Instrumentation to secure SoCs ##

System-on-a-chip (SoC) devices have, recently, achieved considerable importance, because they exist at the heart of virtually every smartphone and tablet manufactured today. SoCs might run software as complex as the Linux kernel; however, given that these devices are usually resource-constrained, their applications tend to be developed in lightweight languages such as C. This fact contributes to making the development of secure applications a very difficult task in the SoC world. Programs developed in type unsafe languages such as C/C++ are particularly vulnerable to a form of information flow vulnerability called buffer-overflow. To aggravate this situation, SoC devices are usually power-constrained; thus, many existing mechanisms used to prevent information flow based attacks are inadequate in their context. Therefore, it is paramount that the research community and the industry put more effort in the development of power efficient strategies that are able to track how unsafe information flows inside a SoC application.
This repository contains different tools that are product of this project:

  * [FlowTracking](FlowTracking.md): is a information flow tracker that is able to uncover implicit information leaks in the program's source code.
  * [unsmasher](unsmasher.md): is a tool that detects code that is vulnerable to buffer overflow attacks, even if it uses canaries to protect functions from return oriented exploits.
  * [ArAnot](ArAnot.md): is a tool that annotates programs with information about the safety of array accesses.
  * [GreenArrays](GreenArrays.md): this is a suite of static analyses that work on top of [AddressSanitizer](http://code.google.com/p/address-sanitizer/wiki/AddressSanitizer). These analyses work together to reduce the overhead imposed by the AddressSanitizer's instrumentation.

  * [SIoT](SIoT.md): is a framework to analyze networked systems. SIoT can crosscheck information inferred from different nodes. This crosschecking increases the precision of traditional static analyses.

This project is sponsored by [Intel Labs](http://www.intel.com)  and the Brazilian Research Council [(CNPq)](http://www.cnpq.br/).