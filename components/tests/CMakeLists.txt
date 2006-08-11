
if(KDE4_BUILD_TESTS)

include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}
)

########### next target ###############

set(agenttypeviewtest_SRCS agenttypeviewtest.cpp )

kde4_automoc(${agenttypeviewtest_SRCS})

kde4_add_executable(agenttypeviewtest RUN_UNINSTALLED ${agenttypeviewtest_SRCS})

target_link_libraries(agenttypeviewtest akonadicomponents ${KDE4_KDECORE_LIBS})

endif(KDE4_BUILD_TESTS)

