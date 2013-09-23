project(akonadi-social-utils)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

include_directories(${QJSON_INCLUDE_DIR})

add_subdirectory(serializer)
add_subdirectory(tests)

set(akonadi_social_utils_SRCS
  socialnetworkattributes.cpp
  imageprovider.cpp
  socialfeeditem.cpp
)

set(akonadi_social_utils_HDRS
  ${CMAKE_CURRENT_BINARY_DIR}/libakonadisocialutils_export.h
  socialnetworkattributes.h
  imageprovider.h
  socialfeeditem.h
)

kde4_add_library(akonadi-socialutils SHARED ${akonadi_social_utils_SRCS})
if(CMAKE_MINIMUM_REQUIRED_VERSION VERSION_LESS 2.8.12)
  set_property(TARGET akonadi-socialutils PROPERTY DEFINE_SYMBOL akonadi_socialutils)
endif()
generate_export_header(akonadi-socialutils
  BASE_NAME libakonadisocialutils
  EXPORT_FILE_NAME libakonadisocialutils_export.h
)

target_link_libraries(akonadi-socialutils
  akonadi-kde
  ${KDE4_KIO_LIBS}
  ${QT_QTNETWORK_LIBS}
  ${QJSON_LIBRARIES}
  ${qjson_LIBRARIES} # for Debian
)

set_target_properties(akonadi-socialutils PROPERTIES
  VERSION ${GENERIC_LIB_VERSION}
  SOVERSION ${GENERIC_LIB_SOVERSION}
)

install(TARGETS akonadi-socialutils EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES ${akonadi_social_utils_HDRS} DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/socialutils)

