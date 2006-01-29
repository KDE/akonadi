INCLUDES = -I$(top_srcdir)/ -I$(top_srcdir)/libakonadi -I$(top_srcdir)/libakonadi/tests $(all_includes)

#AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/libakonadi $(all_includes)
LDADD = ../libakonadi.la -lQtTest_debug

METASOURCES = AUTO
KDE_CXXFLAGS = -UQT3_SUPPORT

# test programs:
check_PROGRAMS = collectionbrowser

collectionbrowser_SOURCES = collectionbrowser.cpp

