
if(KDE4_BUILD_TESTS)

include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}
)

########### next target ###############

set(agentinstanceviewtest_SRCS agentinstanceviewtest.cpp )

kde4_automoc(${agentinstanceviewtest_SRCS})

kde4_add_executable(agentinstanceviewtest RUN_UNINSTALLED ${agentinstanceviewtest_SRCS})

target_link_libraries(agentinstanceviewtest akonadicomponents ${KDE4_KDECORE_LIBS})

########### next target ###############

set(agenttypeviewtest_SRCS agenttypeviewtest.cpp )

kde4_automoc(${agenttypeviewtest_SRCS})

kde4_add_executable(agenttypeviewtest RUN_UNINSTALLED ${agenttypeviewtest_SRCS})

target_link_libraries(agenttypeviewtest akonadicomponents ${KDE4_KDECORE_LIBS})

########### next target ###############

set(profileviewtest_SRCS profileviewtest.cpp )

kde4_automoc(${profileviewtest_SRCS})

kde4_add_executable(profileviewtest RUN_UNINSTALLED ${profileviewtest_SRCS})

target_link_libraries(profileviewtest akonadicomponents ${KDE4_KDECORE_LIBS})

endif(KDE4_BUILD_TESTS)

