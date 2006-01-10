INCLUDES = -I$(top_srcdir)/ $(all_includes)

noinst_LTLIBRARIES = libakonadi.la
libakonadi_la_SOURCES = datarequest.cpp job.cpp query.cpp
libakonadi_la_LDFLAGS = $(all_libraries) -no-undefined
libakonadi_la_LIBADD = $(LIB_QT) $(LIB_KDECORE)

akonadiincludedir = $(includedir)/libakonadi
akonadiinclude_HEADERS = datarequest.h job.h query.h

METASOURCES = AUTO

include $(top_srcdir)/admin/Doxyfile.am
