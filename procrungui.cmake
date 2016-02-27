
# enable/disable cmake debug messages related to this pile
set (PROCRUNGUI_DEBUG_MSG ON)

# make sure support code is present; no harm
# in including it twice; the user, however, should have used
# pileInclude() from pile_support.cmake module.
include(pile_support)

# initialize this module
macro    (procrunguiInit
          ref_cnt_use_mode)

    # default name
    if (NOT PROCRUNGUI_INIT_NAME)
        set(PROCRUNGUI_INIT_NAME "ProcRunGui")
    endif ()

    # compose the list of headers and sources
    set(PROCRUNGUI_HEADERS
        "procrungui.h")
    set(PROCRUNGUI_SOURCES
        "procrungui.cc")

    pileSetSources(
        "${PROCRUNGUI_INIT_NAME}"
        "${PROCRUNGUI_HEADERS}"
        "${PROCRUNGUI_SOURCES}")

    pileSetCommon(
        "${PROCRUNGUI_INIT_NAME}"
        "0;0;1;d"
        "ON"
        "${ref_cnt_use_mode}"
        ""
        "category1"
        "tag1;tag2")

endmacro ()
