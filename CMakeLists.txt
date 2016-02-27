if (NOT PROCRUNGUI_BUILD_MODE)
    set (PROCRUNGUI_BUILD_MODE STATIC)
endif ()

include(pile_support)
pileInclude (ProcRunGui)
procrunguiInit(${PROCRUNGUI_BUILD_MODE})
