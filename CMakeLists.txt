cmake_minimum_required(VERSION 3.16)

project(ZcChat2 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

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
        utils/customscrollbinder.h utils/customscrollbinder.cpp
        windows/tachie/tachie.h windows/tachie/tachie.cpp windows/tachie/tachie.ui
        utils/draghelper.h utils/draghelper.cpp
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
target_include_directories(ZcChat2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcAILib/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ElaWidgetTools/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcWidgetTools/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcJsonLib/include
)

target_link_libraries(ZcChat2 PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcAILib/lib/ZcAiLib.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ElaWidgetTools/lib/elawidgettools.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcWidgetTools/lib/ZcWidgetTools.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ZcJsonLib/lib/ZcJsonLib.lib
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ZcChat2)
endif()
set_target_properties(ZcChat2 PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ZcChat2
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ZcChat2)
endif()
