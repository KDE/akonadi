SUBDIRS = . tests

INCLUDES = -I$(top_srcdir)/ -I$(top_srcdir)/libakonadi $(all_includes)

noinst_LTLIBRARIES = libakonadi.la
libakonadi_la_SOURCES = datarequest.cpp job.cpp query.cpp collection.cpp \
	collectionlistjob.cpp collectionmodel.cpp collectionmonitorjob.cpp \
	collectionfetchjob.cpp collectionview.cpp
libakonadi_la_LDFLAGS = $(all_libraries) -no-undefined
libakonadi_la_LIBADD = $(LIB_QT) $(LIB_KDECORE)

akonadiincludedir = $(includedir)/libakonadi
akonadiinclude_HEADERS = datarequest.h job.h query.h collection.h \
	collectionlistjob.h collectionmodel.h collectionmonitorjob.h \
	collectionfetchjob.h collectionview.h

METASOURCES = AUTO

include $(top_srcdir)/admin/Doxyfile.am
