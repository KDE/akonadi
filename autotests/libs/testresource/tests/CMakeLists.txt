configure_file(${CMAKE_CURRENT_SOURCE_DIR}/knut-empty.xml ${CMAKE_CURRENT_BINARY_DIR}/knut-empty.xml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/knut-step1.xml ${CMAKE_CURRENT_BINARY_DIR}/knut-step1.xml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/knut-step2.xml ${CMAKE_CURRENT_BINARY_DIR}/knut-step2.xml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/knutdemo.xml ${CMAKE_CURRENT_BINARY_DIR}/knutdemo.xml COPYONLY)

akonadi_add_resourcetest( knutdemo knutdemo.js )
