add_subdirectory(agentserver)
add_subdirectory(akonadicontrol)
add_subdirectory(akonadictl)
if(NOT WIN32)
    add_subdirectory(asapcat)
endif()
add_subdirectory(private)
add_subdirectory(interfaces)
add_subdirectory(rds)
add_subdirectory(server)
add_subdirectory(shared)
add_subdirectory(core)
add_subdirectory(agentbase)
add_subdirectory(widgets)
add_subdirectory(selftest)
add_subdirectory(xml)
add_subdirectory(configdialog)
add_subdirectory(quick)
