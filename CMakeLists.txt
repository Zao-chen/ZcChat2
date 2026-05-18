cmake_minimum_required(VERSION 3.16)

project(ZcChat2 VERSION 1.3.0 LANGUAGES CXX)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal binaries")
    list(APPEND CMAKE_FRAMEWORK_PATH "/System/Library/Frameworks")
    add_link_options("-F/System/Library/Frameworks")
endif()

# 配置版本头文件，传递版本号到代码
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Version.h"
)

# 配置 .iss 安装脚本，传递版本号
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/installer/ZcChat2.iss.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ZcChat2.iss"
)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
#find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
#find_package(Qt6 COMPONENTS Network REQUIRED)
#find_package(Qt6 COMPONENTS Multimedia REQUIRED)
#这样更稳妥一点,同时支持qt5和qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Multimedia Svg)
set(QT_VERSION_MAJOR 6)

if(APPLE)
    foreach(qt_target Qt6::Widgets Qt6::Gui Qt6::Core)
        if(TARGET ${qt_target})
            foreach(qt_prop INTERFACE_LINK_LIBRARIES INTERFACE_LINK_OPTIONS)
                get_target_property(qt_values ${qt_target} ${qt_prop})
                if(qt_values)
                    list(FILTER qt_values EXCLUDE REGEX "AGL")
                    set_property(TARGET ${qt_target} PROPERTY ${qt_prop} "${qt_values}")
                endif()
            endforeach()
        endif()
    endforeach()

    foreach(gl_target OpenGL::OpenGL OpenGL::GL OpenGL::GLU)
        if(TARGET ${gl_target})
            foreach(gl_prop INTERFACE_LINK_LIBRARIES INTERFACE_LINK_OPTIONS)
                get_target_property(gl_values ${gl_target} ${gl_prop})
                if(gl_values)
                    list(FILTER gl_values EXCLUDE REGEX "AGL")
                    set_property(TARGET ${gl_target} PROPERTY ${gl_prop} "${gl_values}")
                endif()
            endforeach()
        endif()
    endforeach()

    if(TARGET WrapOpenGL::WrapOpenGL)
        foreach(wrap_prop INTERFACE_LINK_LIBRARIES INTERFACE_LINK_OPTIONS)
            get_target_property(wrap_values WrapOpenGL::WrapOpenGL ${wrap_prop})
            if(wrap_values)
                list(FILTER wrap_values EXCLUDE REGEX "AGL")
                set_property(TARGET WrapOpenGL::WrapOpenGL PROPERTY ${wrap_prop} "${wrap_values}")
            endif()
        endforeach()
    endif()
endif()

# 优先使用外部 ZcAILib 最新构建产物；若不存在则回退到 3rdparty。
set(ZCAILIB_EXTERNAL_ROOT "P:/Qt/Project/ZcAILib")
set(ZCAILIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcAILib/include")
set(ZCAILIB_WIN_LIB "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcAILib/lib/ZcAiLib.lib")
set(ZCAILIB_WIN_DLL "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcAILib/bin/ZcAiLib.dll")

if(EXISTS "${ZCAILIB_EXTERNAL_ROOT}/src/aiprovider.h" AND EXISTS "${ZCAILIB_EXTERNAL_ROOT}/src/ZcAiLib_global.h")
    set(ZCAILIB_INCLUDE_DIR "${ZCAILIB_EXTERNAL_ROOT}/src")
endif()

if(EXISTS "${ZCAILIB_EXTERNAL_ROOT}/build-stream/Release/ZcAiLib.lib" AND EXISTS "${ZCAILIB_EXTERNAL_ROOT}/build-stream/Release/ZcAiLib.dll")
    set(ZCAILIB_WIN_LIB "${ZCAILIB_EXTERNAL_ROOT}/build-stream/Release/ZcAiLib.lib")
    set(ZCAILIB_WIN_DLL "${ZCAILIB_EXTERNAL_ROOT}/build-stream/Release/ZcAiLib.dll")
elseif(EXISTS "${ZCAILIB_EXTERNAL_ROOT}/build/Release/ZcAiLib.lib" AND EXISTS "${ZCAILIB_EXTERNAL_ROOT}/build/Release/ZcAiLib.dll")
    set(ZCAILIB_WIN_LIB "${ZCAILIB_EXTERNAL_ROOT}/build/Release/ZcAiLib.lib")
    set(ZCAILIB_WIN_DLL "${ZCAILIB_EXTERNAL_ROOT}/build/Release/ZcAiLib.dll")
endif()

if(APPLE)
    function(zcchat2_pick_mac_lib out_var dylib_path static_path)
        if(EXISTS "${dylib_path}")
            set(${out_var} "${dylib_path}" PARENT_SCOPE)
        elseif(EXISTS "${static_path}")
            set(${out_var} "${static_path}" PARENT_SCOPE)
        else()
            message(FATAL_ERROR "macOS build requires prebuilt lib: ${dylib_path} or ${static_path}")
        endif()
    endfunction()

    zcchat2_pick_mac_lib(ZCAILIB_MAC_LIB
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcAILib/lib/libZcAiLib.dylib"
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcAILib/lib/libZcAiLib.a")
    zcchat2_pick_mac_lib(ZCJSONLIB_MAC_LIB
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcJsonLib/lib/libZcJsonLib.dylib"
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcJsonLib/lib/libZcJsonLib.a")
    zcchat2_pick_mac_lib(ZCWIDGETTOOLS_MAC_LIB
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcWidgetTools/lib/libZcWidgetTools.dylib"
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcWidgetTools/lib/libZcWidgetTools.a")
    zcchat2_pick_mac_lib(ELAWIDGETTOOLS_MAC_LIB
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ElaWidgetTools/lib/libElaWidgetTools.dylib"
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ElaWidgetTools/lib/libElaWidgetTools.a")
endif()


set(PROJECT_SOURCES
        main.cpp
        windows/setting/setting.cpp
        windows/setting/setting.h
        windows/setting/setting.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ZcChat2
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        windows/dialog/dialog.h windows/dialog/dialog.cpp windows/dialog/dialog.ui
        windows/dialog/history/history.h windows/dialog/history/history.cpp windows/dialog/history/history.ui
        windows/dialog/history/historychild.h windows/dialog/history/historychild.cpp windows/dialog/history/historychild.ui
        GlobalConstants.h
        res.qrc
        utils/CustomScrollBinder.h utils/CustomScrollBinder.cpp
        utils/AnimePlugin.h utils/AnimePlugin.cpp
        utils/AnimePluginManager.h utils/AnimePluginManager.cpp
        windows/tachie/tachie.h windows/tachie/tachie.cpp windows/tachie/tachie.ui
        utils/DragHelper.h utils/DragHelper.cpp
        windows/setting/child/settingchild_llm.h windows/setting/child/settingchild_llm.cpp windows/setting/child/settingchild_llm.ui
        windows/setting/child/settingchild_char.h windows/setting/child/settingchild_char.cpp windows/setting/child/settingchild_char.ui
        windows/setting/child/settingchild_vits.h windows/setting/child/settingchild_vits.cpp windows/setting/child/settingchild_vits.ui
        windows/setting/child/settingchild_speech.h windows/setting/child/settingchild_speech.cpp windows/setting/child/settingchild_speech.ui
        windows/setting/child/settingchild_plugin.h windows/setting/child/settingchild_plugin.cpp windows/setting/child/settingchild_plugin.ui
        windows/setting/child/settingchild_about.h windows/setting/child/settingchild_about.cpp windows/setting/child/settingchild_about.ui
        windows/setting/child/settingchild_general.h windows/setting/child/settingchild_general.cpp windows/setting/child/settingchild_general.ui

    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ZcChat2 APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(ZcChat2 SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(ZcChat2
            ${PROJECT_SOURCES}
        )
    endif()
endif()


# 添加头文件路径
# 1. 统一的头文件路径 (合并原本分散在两处的路径)
target_include_directories(ZcChat2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${ZCAILIB_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcJsonLib/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcWidgetTools/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ElaWidgetTools/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ElaWidgetTools/include/DeveloperComponents
)

# 2. 公共库链接 (全平台通用)
target_link_libraries(ZcChat2 PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Svg
)

if(APPLE)
    target_link_libraries(ZcChat2 PRIVATE
        "${ZCAILIB_MAC_LIB}"
        "${ZCJSONLIB_MAC_LIB}"
        "${ZCWIDGETTOOLS_MAC_LIB}"
        "${ELAWIDGETTOOLS_MAC_LIB}"
        "-framework Cocoa"
        "-framework CoreServices"
    )

    set_target_properties(ZcChat2 PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "ZcChat2"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.zaochen.zcchat2"
        BUILD_RPATH "@executable_path/../Frameworks"
        INSTALL_RPATH "@executable_path/../Frameworks"
    )

    set(ZCCHAT2_MAC_DYLIBS "")
    foreach(mac_lib ${ZCAILIB_MAC_LIB} ${ZCJSONLIB_MAC_LIB} ${ZCWIDGETTOOLS_MAC_LIB} ${ELAWIDGETTOOLS_MAC_LIB})
        if(mac_lib MATCHES "\\.dylib$")
            list(APPEND ZCCHAT2_MAC_DYLIBS "${mac_lib}")
        endif()
    endforeach()

    if(ZCCHAT2_MAC_DYLIBS)
        add_custom_command(TARGET ZcChat2 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_BUNDLE_DIR:ZcChat2>/Contents/Frameworks"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${ZCCHAT2_MAC_DYLIBS}
                "$<TARGET_BUNDLE_DIR:ZcChat2>/Contents/Frameworks"
            VERBATIM
        )
    endif()
endif()

# 3. Windows 专用链接
if(WIN32)
    target_link_libraries(ZcChat2 PRIVATE
        ${ZCAILIB_WIN_LIB}
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ElaWidgetTools/lib/elawidgettools.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcWidgetTools/lib/ZcWidgetTools.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcJsonLib/lib/ZcJsonLib.lib
    )

    add_custom_command(TARGET ZcChat2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${ZCAILIB_WIN_DLL}
            $<TARGET_FILE_DIR:ZcChat2>/ZcAiLib.dll
    )
endif()

# 4. Linux 专用链接
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED COMPONENTS Xext)
    if(QT_VERSION_MAJOR EQUAL 5)
        find_package(Qt5 REQUIRED COMPONENTS X11Extras)
    endif()

    target_link_libraries(ZcChat2 PRIVATE
        ${X11_LIBRARIES}
        ${X11_Xext_LIB}
        $<$<STREQUAL:${QT_VERSION_MAJOR},5>:Qt5::X11Extras>
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcAILib/lib/libZcAiLib.a"
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcJsonLib/lib/libZcJsonLib.a"
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcWidgetTools/lib/libZcWidgetTools.a"
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ElaWidgetTools/lib/libElaWidgetTools.a"
    )
    
    set_target_properties(ZcChat2 PROPERTIES
        INSTALL_RPATH "$ORIGIN/3rdparty/ZcJsonLib/lib:$ORIGIN/3rdparty/ZcAILib/lib:$ORIGIN/3rdparty/ZcWidgetTools/lib:$ORIGIN/3rdparty/ElaWidgetTools/lib"
    )
endif()

include(GNUInstallDirs)
install(TARGETS ZcChat2
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ZcChat2)
endif()
