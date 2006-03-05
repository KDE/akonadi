kde4_header()

add_subdirectory( tests ) 

include_directories( ${CMAKE_SOURCE_DIR}/ ${CMAKE_SOURCE_DIR}/libakonadi ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############


########### install files ###############

install_files( /include/libakonadi FILES datarequest.h job.h query.h collection.h  	collectionlistjob.h collectionmodel.h collectionmonitorjob.h  	collectionfetchjob.h collectionview.h messagecollection.h  	messagecollectionmodel.h )

kde4_create_doxygen_docs(  )

kde4_footer()



#original Makefile.am contents follow:

#SUBDIRS = . tests
#
#INCLUDES = -I$(top_srcdir)/ -I$(top_srcdir)/libakonadi $(all_includes)
#
#noinst_LTLIBRARIES = libakonadi.la
#libakonadi_la_SOURCES = datarequest.cpp job.cpp query.cpp collection.cpp \
#	collectionlistjob.cpp collectionmodel.cpp collectionmonitorjob.cpp \
#	collectionfetchjob.cpp collectionview.cpp messagecollection.cpp \
#	messagecollectionmodel.cpp
#libakonadi_la_LDFLAGS = $(all_libraries) -no-undefined
#libakonadi_la_LIBADD = $(LIB_QT) $(LIB_KDECORE)
#
#akonadiincludedir = $(includedir)/libakonadi
#akonadiinclude_HEADERS = datarequest.h job.h query.h collection.h \
#	collectionlistjob.h collectionmodel.h collectionmonitorjob.h \
#	collectionfetchjob.h collectionview.h messagecollection.h \
#	messagecollectionmodel.h
#
#METASOURCES = AUTO
#
#include $(top_srcdir)/admin/Doxyfile.am
