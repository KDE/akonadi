# SPDX-License-Identifier: BSD-3-Clause

function(embed_info_plist target identifier name)
    set_target_properties(
        ${target}
        PROPERTIES
            MACOSX_BUNDLE
                FALSE
    )

    get_target_property(target_name ${target} NAME)

    set(plist_path "${CMAKE_CURRENT_BINARY_DIR}/${target_name}.Info.plist")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER ${identifier})
    set(MACOSX_BUNDLE_BUNDLE_NAME ${name})
    configure_file(${CMAKE_SOURCE_DIR}/Info.plist.in ${plist_path})

    target_link_options(${target} PRIVATE "LINKER:SHELL:-sectcreate __TEXT __info_plist \"${plist_path}\"")

    set_property(
        TARGET
            ${target}
        APPEND
        PROPERTY
            LINK_DEPENDS
                "${plist_path}"
    )
endfunction()
