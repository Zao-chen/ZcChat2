cmake_minimum_required(VERSION 3.16)

project(ZcChat2 VERSION 0.1 LANGUAGES CXX)

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
        GlobalConstants.h
        res.qrc
        utils/CustomScrollBinder.h utils/CustomScrollBinder.cpp
        windows/tachie/tachie.h windows/tachie/tachie.cpp windows/tachie/tachie.ui
        utils/DragHelper.h utils/DragHelper.cpp
        windows/setting/child/settingchild_llm.h windows/setting/child/settingchild_llm.cpp windows/setting/child/settingchild_llm.ui
        windows/setting/child/settingchild_char.h windows/setting/child/settingchild_char.cpp windows/setting/child/settingchild_char.ui
        windows/setting/child/settingchild_vits.h windows/setting/child/settingchild_vits.cpp windows/setting/child/settingchild_vits.ui

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
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcAILib/include
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

# 3. Windows 专用链接
if(WIN32)
    target_link_libraries(ZcChat2 PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcAILib/lib/ZcAiLib.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ElaWidgetTools/lib/elawidgettools.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcWidgetTools/lib/ZcWidgetTools.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcJsonLib/lib/ZcJsonLib.lib
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
