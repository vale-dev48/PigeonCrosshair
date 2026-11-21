cmake_minimum_required(VERSION 3.21)
project(PigeonCrosshair VERSION 1.0.0 LANGUAGES CXX)
if(WIN32)
    enable_language(RC)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Svg)

qt_add_executable(PigeonCrosshair WIN32
    src/main.cpp
    src/AppController.h        src/AppController.cpp
    src/CrosshairConfig.h      src/CrosshairConfig.cpp
    src/CrosshairRenderer.h    src/CrosshairRenderer.cpp
    src/ConfigStore.h          src/ConfigStore.cpp
    src/OverlayWindow.h        src/OverlayWindow.cpp
    src/PreviewWidget.h        src/PreviewWidget.cpp
    src/Widgets.h              src/Widgets.cpp
    src/SettingsWindow.h       src/SettingsWindow.cpp
    src/TrayController.h       src/TrayController.cpp
    resources/resources.qrc
)
if(WIN32)
    target_sources(PigeonCrosshair PRIVATE resources/app.rc)   # icona dell'exe
endif()

target_include_directories(PigeonCrosshair PRIVATE src)
target_link_libraries(PigeonCrosshair PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg)

if(WIN32)
    target_compile_definitions(PigeonCrosshair PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(PigeonCrosshair PRIVATE user32)

    # Copia DLL/plugin Qt accanto all'exe (windeployqt) per poterlo lanciare da Esplora Risorse.
    option(PC_DEPLOY_QT "Esegue windeployqt dopo la build" ON)
    if(PC_DEPLOY_QT)
        get_target_property(_qmake Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(_qtbin "${_qmake}" DIRECTORY)
        find_program(WINDEPLOYQT windeployqt HINTS "${_qtbin}")
        if(WINDEPLOYQT)
            add_custom_command(TARGET PigeonCrosshair POST_BUILD
                COMMAND "${WINDEPLOYQT}" --no-translations --no-system-d3d-compiler
                        --no-opengl-sw "$<TARGET_FILE:PigeonCrosshair>"
                COMMENT "windeployqt")
        endif()
    endif()
endif()
