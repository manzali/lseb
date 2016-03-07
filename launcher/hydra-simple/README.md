Import of hydra-simple files
============================

The files from this directory are imported from the mpich sources :

 - src/pmi/simple/simple_pmi.c
 - src/pmi/simple/simple_pmiutil.c
 - src/pmi/simple/simple_pmiutil.h
 - src/include/pmi.h
 - src/include/mpimem.h
 - src/include/mpibase.h
 - src/pm/util/safestr2.c

Some changes have been done on those files :

 - mpimem.h : only kept the two needed function, so its mostly not the original file.
 - simple_pmi.c : replace include of mpi.h by #define MPI_MAX_PORT_NAME 256
 - In all files, replace the mpichconf.h file by pmiconfig.h

Most of this is inspired from what does the MPC (http://mpc.paratools.com/) project to use hydra.

This directory also require an installation of hydra from mpich (https://www.mpich.org/downloads/)
but the default archive must be modified to install the inner library `libmpl.so` as it is not by 
default. It can be done easily with the changes :

```diff
diff --git a/mpl/Makefile.am b/mpl/Makefile.am
index 355f102..5800349 100644
--- a/mpl/Makefile.am
+++ b/mpl/Makefile.am
@@ -7,7 +7,7 @@
 ACLOCAL_AMFLAGS = -I confdb
 AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include
 
-noinst_LTLIBRARIES = lib@MPLLIBNAME@.la
+lib_LTLIBRARIES = lib@MPLLIBNAME@.la
 lib@MPLLIBNAME@_la_SOURCES = src/mplstr.c src/mpltrmem.c src/mplenv.c src/mplsock.c
 lib@MPLLIBNAME@_la_LDFLAGS = ${lib@MPLLIBNAME@_so_versionflags}
```