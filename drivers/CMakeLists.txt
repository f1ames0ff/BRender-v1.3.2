add_subdirectory(softrend)
add_subdirectory(pentprim)
add_subdirectory(virtual_fb)
add_subdirectory(glrend)

target_link_libraries(brender-full INTERFACE softrend pentprim virtual_fb glrend )

if(BRENDER_BUILD_VKREND)
    add_subdirectory(vkrend)
    target_link_libraries(brender-full INTERFACE vkrend )
endif()
