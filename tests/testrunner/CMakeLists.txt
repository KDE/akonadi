project (akonaditest)

include_directories(${KDE4_INCLUDES})
include_directories (${Boost_INCLUDE_DIR})
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

set(akonaditest_SRCS main.cpp akonaditesting.cpp dao.cpp  item.cpp itemfactory.cpp  calitem.cpp vcarditem.cpp feeditem.cpp setup.cpp configreader.cpp config.cpp shellscript.cpp symbols.cpp)

kde4_add_executable(akonaditest ${akonaditest_SRCS})
target_link_libraries(akonaditest ${KDE4_KDEUI_LIB} ${Boost_LIBRARIES} ${KDE4_AKONADI_LIBS} ${KDE4_KABC_LIBS} ${KDE4_KCAL_LIBS} ${KDE4_SYNDICATION_LIBS} )
install(TARGETS akonaditest ${INSTALL_TARGETS_DEFAULT_ARGS})
