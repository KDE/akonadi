
add_subdirectory( tests )

include_directories(
	${CMAKE_SOURCE_DIR}/
	${CMAKE_SOURCE_DIR}/libakonadi
	${CMAKE_SOURCE_DIR}/libkmime

)


########### next target ###############

set( akonadi_LIB_SRC
	datarequest.cpp
	job.cpp
	query.cpp
	collection.cpp
	collectionlistjob.cpp
	collectionmodel.cpp
	collectionfetchjob.cpp
	collectionview.cpp
	messagecollection.cpp
	messagecollectionmodel.cpp
	message.cpp
	messagemodel.cpp
	messagequery.cpp
	monitor.cpp
)

kde4_automoc( ${akonadi_LIB_SRC} )
kde4_add_library( akonadi SHARED ${akonadi_LIB_SRC} )
target_link_libraries( akonadi ${QT_QTNETWORK_LIBRARY} ${KDE4_KDECORE_LIBS} kmime )
set_target_properties( akonadi PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install_targets( ${LIB_INSTALL_DIR} akonadi )

########### install files ###############

install_files( /include/libakonadi FILES
	datarequest.h
	job.h
	query.h
	collection.h
	collectionlistjob.h
	collectionmodel.h
	collectionfetchjob.h
	collectionview.h
	messagecollection.h
	messagecollectionmodel.h
	message.h
	messagemodel.h
	messagequery.h
	monitor.h
)


