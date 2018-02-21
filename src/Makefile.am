SUBDIRS = lib/
bin_PROGRAMS = bdl

bdl_LDFLAGS = -Wl,--no-as-needed -ldl -rdynamic
bdl_LDADD = lib/libbdl.la
bdl_SOURCES = bdl.c
