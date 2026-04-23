cmake_minimum_required(VERSION 3.16)
project(gta2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect all source files
file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

if(WIN32)
    # ---- Windows ----
    set(SDL2_BASE "${CMAKE_SOURCE_DIR}/libs/SDL2_win")

    if(MINGW)
        # MinGW layout: x86_64-w64-mingw32/{include/SDL2, lib, bin}
        set(SDL2_PREFIX     "${SDL2_BASE}/SDL2-2.30.12/x86_64-w64-mingw32")
        set(SDL2_IMG_PREFIX "${SDL2_BASE}/SDL2_image-2.8.4/x86_64-w64-mingw32")
        set(SDL2_MIX_PREFIX "${SDL2_BASE}/SDL2_mixer-2.8.1/x86_64-w64-mingw32")
        set(SDL2_TTF_PREFIX "${SDL2_BASE}/SDL2_ttf-2.22.0/x86_64-w64-mingw32")

        target_include_directories(${PROJECT_NAME} PRIVATE
            "${SDL2_PREFIX}/include/SDL2"
            "${SDL2_IMG_PREFIX}/include/SDL2"
            "${SDL2_MIX_PREFIX}/include/SDL2"
            "${SDL2_TTF_PREFIX}/include/SDL2"
        )

        target_link_directories(${PROJECT_NAME} PRIVATE
            "${SDL2_PREFIX}/lib"
            "${SDL2_IMG_PREFIX}/lib"
            "${SDL2_MIX_PREFIX}/lib"
            "${SDL2_TTF_PREFIX}/lib"
        )

        target_link_libraries(${PROJECT_NAME} PRIVATE
            mingw32
            SDL2main SDL2
            SDL2_image
            SDL2_mixer
            SDL2_ttf
        )

        # Copy DLLs from bin/ directories
        set(DLL_DIRS
            "${SDL2_PREFIX}/bin"
            "${SDL2_IMG_PREFIX}/bin"
            "${SDL2_MIX_PREFIX}/bin"
            "${SDL2_TTF_PREFIX}/bin"
        )

        foreach(DLL_DIR ${DLL_DIRS})
            file(GLOB DLL_FILES "${DLL_DIR}/*.dll")
            foreach(DLL_FILE ${DLL_FILES})
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${DLL_FILE}"
                        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                )
            endforeach()
        endforeach()

    else()
        # MSVC layout: include/, lib/x64/
        set(SDL2_DIR      "${SDL2_BASE}/SDL2-2.30.12")
        set(SDL2_IMG_DIR  "${SDL2_BASE}/SDL2_image-2.8.4")
        set(SDL2_MIX_DIR  "${SDL2_BASE}/SDL2_mixer-2.8.1")
        set(SDL2_TTF_DIR  "${SDL2_BASE}/SDL2_ttf-2.22.0")

        target_include_directories(${PROJECT_NAME} PRIVATE
            "${SDL2_DIR}/include"
            "${SDL2_IMG_DIR}/include"
            "${SDL2_MIX_DIR}/include"
            "${SDL2_TTF_DIR}/include"
        )

        target_link_directories(${PROJECT_NAME} PRIVATE
            "${SDL2_DIR}/lib/x64"
            "${SDL2_IMG_DIR}/lib/x64"
            "${SDL2_MIX_DIR}/lib/x64"
            "${SDL2_TTF_DIR}/lib/x64"
        )

        target_link_libraries(${PROJECT_NAME} PRIVATE
            SDL2 SDL2main
            SDL2_image
            SDL2_mixer
            SDL2_ttf
        )
    endif()

else()
    # ---- Linux ----
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
    pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_IMAGE_INCLUDE_DIRS}
        ${SDL2_MIXER_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${SDL2_LIBRARIES}
        ${SDL2_IMAGE_LIBRARIES}
        ${SDL2_MIXER_LIBRARIES}
        ${SDL2_TTF_LIBRARIES}
    )

    target_compile_options(${PROJECT_NAME} PRIVATE
        ${SDL2_CFLAGS_OTHER}
    )
endif()

# Copy assets to build output so the exe can find them
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
    COMMENT "Copying assets to output directory"
)
