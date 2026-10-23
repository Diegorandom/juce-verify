cmake_minimum_required(VERSION 3.22)

project(juce-verify VERSION 0.1.0 LANGUAGES C CXX)

# El curso fija JUCE 8.0.16-security. La ruta se puede sobreescribir para que el
# arnés siga sirviendo cuando el pin cambie:
#   cmake -S . -B build -DJUCE_DIR=/ruta/a/JUCE
set(JUCE_DIR "" CACHE PATH "Raíz del árbol de JUCE (obligatorio: -DJUCE_DIR=/ruta/a/JUCE)")

if(JUCE_DIR STREQUAL "" OR NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "Falta JUCE. Descarga un árbol de JUCE y pásalo por ruta:\n"
        "  cmake -S . -B build -DJUCE_DIR=/ruta/a/JUCE")
endif()

add_subdirectory("${JUCE_DIR}" JUCE)

juce_add_console_app(juce-verify PRODUCT_NAME "juce-verify")

# Se enlaza el módulo *headless* a propósito: el arnés corre en consola, sin
# ventana ni editor. Ese es exactamente el caso de uso para el que JUCE separó
# juce_audio_processors_headless, y así se verifica también que el traslado de
# AudioProcessor.h a ese módulo no rompe nada.
target_sources(juce-verify PRIVATE src/main.cpp)

target_compile_definitions(juce-verify PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:juce-verify,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:juce-verify,JUCE_VERSION>"
)

target_link_libraries(juce-verify PRIVATE
    juce::juce_audio_processors_headless
    juce::juce_audio_basics
    juce::juce_dsp
    juce::juce_events
    juce::juce_core
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
