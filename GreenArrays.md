# GreenArrays #

The C programming language is notoriously unsafe. As an example, it does not prevent out-of-bounds memory accesses. There exist several techniques to secure C programs. However, although effective, these methods tend to slow down the safe programs substantially. The GreenArrays project provides a suite of compiler analyses, implemented on top of [LLVM](http://llvm.org/).

  * [Overview](ga_overview.md): our framework in one picture!

  * [Download](ga_download.md): download, install and run green arrays.

  * [Symbolic ranges](ga_sra_gallery.md): examples of use of the symbolic range analysis.

  * [Region analysis](ga_reg_gallery.md): examples of use of the region analysis.

  * [Using Symbolic Ranges](ga_client_sra.md): we show how to build an LLVM pass that uses our analysis.

Our framework consists in four analyses: Symbolic Range Analysis,
Region Analysis, Tainted Flow Analysis, and integer overflow analysis. The last two are well-known in the compiler community. The first two are not **completely** new also. People have described them in previous work. Yet, we believe that our design and implementation of these technologies advances substantially the previous work in the area.