project(socialfeeditemserializer)

if(WIN32)
  set(
    LIB_INSTALL_DIR ${LIB_INSTALL_DIR}
    RUNTIME DESTINATION ${BIN_INSTALL_DIR}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
  )
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

include_directories(
  ${CMAKE_BINARY_DIR}/akonadi/socialutils
)

########### next target ###############

set(akonadi_serializer_socialfeeditem_SRCS
  akonadi_serializer_socialfeeditem.cpp
)

kde4_add_plugin(akonadi_serializer_socialfeeditem ${akonadi_serializer_socialfeeditem_SRCS})

target_link_libraries(akonadi_serializer_socialfeeditem
  akonadi-socialutils
  akonadi-kde
  ${QT_QTCORE_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${qjson_LIBRARIES}    # for Debian
  ${QJSON_LIBRARIES})

install(TARGETS akonadi_serializer_socialfeeditem DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install(FILES
  akonadi_serializer_socialfeeditem.desktop
  DESTINATION ${DATA_INSTALL_DIR}/akonadi/plugins/serializer)

install(FILES x-vnd.akonadi.socialfeeditem.xml DESTINATION ${XDG_MIME_INSTALL_DIR})

update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})
