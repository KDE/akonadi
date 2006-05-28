
if(KDE4_BUILD_TESTS)

include_directories(
	${CMAKE_SOURCE_DIR}/libakonadi
	${CMAKE_SOURCE_DIR}/libakonadi/tests
	${CMAKE_SOURCE_DIR}/libkmime
)


########### next target ###############

set(collectionbrowser_SRCS collectionbrowser.cpp messagebrowser.cpp )

kde4_automoc(${collectionbrowser_SRCS})

kde4_add_executable(collectionbrowser RUN_UNINSTALLED ${collectionbrowser_SRCS})

target_link_libraries(collectionbrowser  akonadi ${KDE4_KDECORE_LIBS} )

########### next target ###############

set(messagebrowser_SRCS messagebrowser.cpp  messagebrowser_main.cpp )

kde4_automoc(${messagebrowser_SRCS})

kde4_add_executable(messagebrowser RUN_UNINSTALLED ${messagebrowser_SRCS})

target_link_libraries(messagebrowser  akonadi ${KDE4_KDECORE_LIBS} )

########### next target ###############

set(itemdumper_SRCS itemdumper.cpp )

kde4_automoc(${itemdumper_SRCS})

kde4_add_executable(itemdumper RUN_UNINSTALLED ${itemdumper_SRCS})

target_link_libraries(itemdumper  akonadi ${KDE4_KDECORE_LIBS} )


endif(KDE4_BUILD_TESTS)

