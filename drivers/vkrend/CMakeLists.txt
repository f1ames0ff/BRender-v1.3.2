project(vkrend C)

set(VKREND_FILES
        driver.c
        device.c
        device.h
        drv.h
        drv_ip.h
        template.h
        )

set(INC_FILES
        include/brvkrend.h
        include/brvkrend_p.h
        )

add_library(vkrend-headers INTERFACE)
target_sources(vkrend-headers INTERFACE
        FILE_SET include
        TYPE HEADERS
        BASE_DIRS include
        FILES ${INC_FILES}
        )

target_include_directories(vkrend-headers INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/brender/vkrend>
        )

set_target_properties(vkrend-headers PROPERTIES
        EXPORT_NAME "Drivers::VkRend::Headers"
        FOLDER "BRender/Drivers"
        )

add_library(vkrend
        ${VKREND_FILES}
        ${INC_FILES}
        )

get_target_property(target_type vkrend TYPE)
if(target_type STREQUAL SHARED_LIBRARY)
    target_compile_definitions(vkrend PRIVATE -DDEFINE_BR_ENTRY_POINT)
endif()

target_link_libraries(vkrend PRIVATE BRender::DDI)
target_link_libraries(vkrend PUBLIC vkrend-headers)

set_target_properties(vkrend PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED TRUE
        EXPORT_NAME "Drivers::VkRend"
        FOLDER "BRender/Drivers"
        )

add_library(BRender::Drivers::VkRend::Headers ALIAS vkrend-headers)
add_library(BRender::Drivers::VkRend ALIAS vkrend)
