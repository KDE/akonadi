set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}
)

########### next target ###############

set(agentinstanceviewtest_SRCS agentinstanceviewtest.cpp )


kde4_add_executable(agentinstanceviewtest TEST  ${agentinstanceviewtest_SRCS})

target_link_libraries(agentinstanceviewtest akonadicomponents ${KDE4_KDECORE_LIBS})

########### next target ###############

set(agenttypeviewtest_SRCS agenttypeviewtest.cpp )


kde4_add_executable(agenttypeviewtest TEST  ${agenttypeviewtest_SRCS})

target_link_libraries(agenttypeviewtest akonadicomponents ${KDE4_KDECORE_LIBS})

########### next target ###############

set(profileviewtest_SRCS profileviewtest.cpp )


kde4_add_executable(profileviewtest TEST  ${profileviewtest_SRCS})

target_link_libraries(profileviewtest akonadicomponents ${KDE4_KDECORE_LIBS})


