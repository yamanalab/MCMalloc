# MCMalloc: A Scalable Memory Allocator for Multithreaded Applications on a Many-Core Shared-Memory Machine

MCMalloc library enables to control local heaps to speed up
memory allocation and deallocation by batch malloc, pseudo free, and fine-grained data-locking.
(This library is updated version of FCMalloc.)

[MCMalloc: A scalable memory allocator for multithreaded applications on a many\-core shared\-memory machine \- IEEE Conference Publication]( http://ieeexplore.ieee.org/document/8258563/metrics )

## how to build
Ninja, GCC are required to build MCMalloc library.

```
$ ninja
```


## how to run
In order to use MCMalloc library, set environment variable `LD_PRELOAD`
to the path to `libmcmalloc.so`. For example, zsh uses MCMalloc library;
e.g.
```
LD_PRELOAD=./libmcmalloc.so zsh
```


## NOTE
* The following functions are unsupported.
    * pvalloc
    * mallopt
* Memory allocated within libmcmalloc.so is not released
  unless the process is terminated.


## References
* TCMalloc
    * Google Inc., "gperftools: Fast, multi-threaded malloc() and nifty performance analysis tools ", http://code.google.com/p/gperftools/ accessed on 2017-03-15.
* JEmalloc
    * Evans, J., "A Scalable Concurrent malloc(3) Implementation for FreeBSD", Proc. of BSDCan, 2006.
* Supermalloc
    * Kuszmaul, B. C., "Supermalloc: A Super Fast Multithreaded Malloc for 64-bit Machines", Proc. of ISMM, pp. 41-55, 2015.
* FHE
    * Gentry, C., "A Fully Homomorphic Encryption Scheme", Ph.D. Thesis, Stanford University, 2009.
* HElib
    * http://shaih.github.io/HElib/
