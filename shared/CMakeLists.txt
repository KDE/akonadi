set(akonadi_shared_srcs
  akapplication.cpp
  akcrash.cpp
  akdbus.cpp
  akdebug.cpp
  akstandarddirs.cpp
)

add_library(akonadi_shared STATIC ${akonadi_shared_srcs})

target_link_libraries(akonadi_shared ${AKONADI_SYSTEM_LIBS})
