function(TargetLinkGraphicAPIWith TARGET_NAME)
    if(NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "Link Graphic API Error: Target ${TARGET_NAME} not found")
    endif()

    # Vulkan
    find_package(Vulkan QUIET)
    if(TARGET Vulkan::Vulkan)
        message(STATUS "Find Vulkan and use it in Target ${TARGET_NAME}")
        target_link_libraries(${TARGET_NAME} PRIVATE Vulkan::Vulkan)
        if(DEFINED Vulkan_INCLUDE_DIR)
            message(STATUS "Link Vulkan Include Dir where is ${Vulkan_INCLUDE_DIR}")
            target_include_directories(${TARGET_NAME} PRIVATE ${Vulkan_INCLUDE_DIR})
            set_property(TARGET ${TARGET_NAME} PROPERTY GRAPHICS_BACKEND "Vulkan")
            return()
        else()
            message(FATAL_ERROR "Vulkan Include Dir not found")
            return()
        endif()
    elseif(DEFINED Vulkan_INCLUDE_DIR OR DEFINED ENV{VULKAN_SDK})
        message(FATAL_ERROR "Vulkan Include Dir found, but the funciotn is testing")
        # Try constructing an imported Vulkan target from VULKAN_SDK if find_package didn't create one
        if(NOT DEFINED Vulkan_INCLUDE_DIR AND DEFINED ENV{VULKAN_SDK})
            set(Vulkan_INCLUDE_DIR "$ENV{VULKAN_SDK}/Include")
        endif()

        if(WIN32)
            if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                set(_VULKAN_LIB_CAND "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib")
            else()
                set(_VULKAN_LIB_CAND "$ENV{VULKAN_SDK}/Lib32/vulkan-1.lib")
            endif()
        elseif(APPLE)
            set(_VULKAN_LIB_CAND "$ENV{VULKAN_SDK}/lib/libvulkan.1.dylib")
        else()
            set(_VULKAN_LIB_CAND "$ENV{VULKAN_SDK}/lib/libvulkan.so")
        endif()

        if(DEFINED _VULKAN_LIB_CAND AND EXISTS "${_VULKAN_LIB_CAND}")
            add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
            set_target_properties(Vulkan::Vulkan PROPERTIES
                IMPORTED_LOCATION "${_VULKAN_LIB_CAND}"
                INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIR}"
            )
            message(STATUS "[GraphicsBackend] Using Vulkan (from VULKAN_SDK) include='${Vulkan_INCLUDE_DIR}' lib='${_VULKAN_LIB_CAND}'")
            target_link_libraries(${TARGET_NAME} PRIVATE Vulkan::Vulkan)
            set_property(TARGET ${TARGET_NAME} PROPERTY GRAPHICS_BACKEND "Vulkan")
            return()
        endif()
    endif()

    # DirectX 12
    find_package(d3d12 QUIET)
    if(d3d12_FOUND)
        message(STATUS "Find DirectX 12 and use it in Target ${TARGET_NAME}")

        if(DEFINED d3d12_INCLUDE_DIRS)
            message(STATUS "Link DirectX 12 Include Dir where is ${d3d12_INCLUDE_DIRS}")
            target_include_directories(${TARGET_NAME} PRIVATE ${d3d12_INCLUDE_DIRS})
        endif()

        target_link_libraries(${TARGET_NAME} PRIVATE d3d12)
        set_property(TARGET ${TARGET_NAME} PROPERTY GRAPHICS_BACKEND "D3D12")
        return()
    endif()

    # DirectX 11
    find_package(d3d11 QUIET)

    if(d3d11_FOUND)
        message(STATUS "Find DirectX 11 and use it in Target ${TARGET_NAME}")

        if(DEFINED d3d11_INCLUDE_DIRS)
            message(STATUS "Link DirectX 11 Include Dir where is ${d3d11_INCLUDE_DIRS}")
            target_include_directories(${TARGET_NAME} PRIVATE ${d3d11_INCLUDE_DIRS})
        endif()

        target_link_libraries(${TARGET_NAME} PRIVATE d3d11)
        set_property(TARGET ${TARGET_NAME} PROPERTY GRAPHICS_BACKEND "D3D11")
        return()
    endif()

    # Metal
    find_library(METAL_FRAMEWORK Metal)

    if(METAL_FRAMEWORK)
        message(STATUS "Find Metal and use it in Target ${TARGET_NAME}")
        target_link_libraries(${TARGET_NAME} PRIVATE ${METAL_FRAMEWORK})
        set_property(TARGET ${TARGET_NAME} PROPERTY GRAPHICS_BACKEND "Metal")
        return()
    endif()

    # OpenGL
    find_package(OpenGL QUIET)

    if(OpenGL_FOUND OR TARGET OpenGL::GL)
        message(STATUS "Find OpenGL and use it in Target ${TARGET_NAME}")

        if(DEFINED OpenGL_INCLUDE_DIR)
            message(STATUS "Link OpenGL Include Dir where is ${OpenGL_INCLUDE_DIR}")
            target_include_directories(${TARGET_NAME} PRIVATE ${OpenGL_INCLUDE_DIR})
        endif()

        if(TARGET OpenGL::GL)
            target_link_libraries(${TARGET_NAME} PRIVATE OpenGL::GL)
        elseif(DEFINED OpenGL_gl_LIBRARY)
            target_link_libraries(${TARGET_NAME} PRIVATE ${OpenGL_gl_LIBRARY})
        endif()

        set_property(TARGET ${TARGET_NAME} PROPERTY GRAPHICS_BACKEND "OpenGL")
        return()
    endif()
    message(FATAL_ERROR "No suitable graphics backend found. Tried: Vulkan, OpenGL")

endfunction()
