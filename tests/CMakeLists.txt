kde4_header()

include_directories(
	${CMAKE_SOURCE_DIR}/libakonadi
	${CMAKE_SOURCE_DIR}/libakonadi/tests
	${KDE4_INCLUDES}
)


########### next target ###############

set(collectionbrowser_SRCS collectionbrowser.cpp )

kde4_automoc(${collectionbrowser_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(collectionbrowser ${collectionbrowser_SRCS})

target_link_libraries(collectionbrowser  akonadi ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### next target ###############

set(messagebrowser_SRCS messagebrowser.cpp )

kde4_automoc(${messagebrowser_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(messagebrowser ${messagebrowser_SRCS})

target_link_libraries(messagebrowser  akonadi ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#INCLUDES = -I$(top_srcdir)/ -I$(top_srcdir)/libakonadi -I$(top_srcdir)/libakonadi/tests $(all_includes)
#
##AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/libakonadi $(all_includes)
#LDADD = ../libakonadi.la -lQtTest_debug
#
#METASOURCES = AUTO
#KDE_CXXFLAGS = -UQT3_SUPPORT
#
## test programs:
#check_PROGRAMS = collectionbrowser
#
#collectionbrowser_SOURCES = collectionbrowser.cpp
#
