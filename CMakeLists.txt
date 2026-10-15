cmake_minimum_required(VERSION 3.22)

project(QuadrumI VERSION 0.1.0 LANGUAGES CXX)

# Nome do produto num único lugar: executável, pastas de dados e textos.
# Para a versão whitelabel, basta trocar estes dois valores.
set(QUADRUM_DISPLAY_NAME "Quadrum I")
set(QUADRUM_APP_ID "QuadrumI")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick Sql LinguistTools)
find_package(SDL3 CONFIG REQUIRED) # via vcpkg (vcpkg.json)
qt_standard_project_setup(REQUIRES 6.8)

configure_file(src/app/AppInfo.h.in ${CMAKE_CURRENT_BINARY_DIR}/generated/AppInfo.h @ONLY)

# Núcleo: regras do hub, sem interface. Testado sozinho em tests/.
qt_add_library(quadrum_core STATIC
    src/core/Paths.h src/core/Paths.cpp
    src/core/Logger.h src/core/Logger.cpp
    src/core/Database.h src/core/Database.cpp
    src/core/ProfileStore.h src/core/ProfileStore.cpp
)

target_include_directories(quadrum_core PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    src
)

target_link_libraries(quadrum_core PUBLIC
    Qt6::Core
    Qt6::Sql
)

# Aplicativo: interface QML e as pontes entre ela e o núcleo.
qt_add_executable(quadrum
    src/app/main.cpp
)

set_target_properties(quadrum PROPERTIES
    OUTPUT_NAME "${QUADRUM_APP_ID}"
    WIN32_EXECUTABLE TRUE
)

# O registro de tipos QML gerado pelo Qt inclui os headers só pelo nome.
target_include_directories(quadrum PRIVATE src/ui src/input)

set_source_files_properties(qml/theme/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(quadrum
    URI Quadrum
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/theme/Theme.qml
    SOURCES
        src/ui/ThemeService.h src/ui/ThemeService.cpp
        src/ui/LanguageService.h src/ui/LanguageService.cpp
        src/ui/ProfileModel.h src/ui/ProfileModel.cpp
        src/ui/ProfileService.h src/ui/ProfileService.cpp
        src/input/GamepadService.h src/input/GamepadService.cpp
)

qt_add_translations(quadrum
    TS_FILES i18n/quadrum_en.ts
    RESOURCE_PREFIX /i18n
)

target_link_libraries(quadrum PRIVATE
    quadrum_core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    SDL3::SDL3
)

enable_testing()
add_subdirectory(tests)
