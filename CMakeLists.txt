# Versioning
project(serverbrige VERSION 0.1.0 LANGUAGES CXX)

# Sources
include(cmake/sources.cmake)

# Target
if(SERVERBRIGE_COMPILE_SOURCES)
    add_library(serverbrige
        ${SERVERBRIGE_SOURCES}
    )
    set(SERVERBRIGE_TARGET_SCOPE PUBLIC)
else()
    add_library(serverbrige INTERFACE)
    set(SERVERBRIGE_TARGET_SCOPE INTERFACE)
endif()

target_compile_features(serverbrige ${SERVERBRIGE_TARGET_SCOPE} cxx_std_17)

# Angle Includes
include(cmake/include.cmake)

target_include_directories(serverbrige
    ${SERVERBRIGE_TARGET_SCOPE}
        ${SERVERBRIGE_INCLUDE}
)

# Linkage
target_link_libraries(serverbrige
    ${SERVERBRIGE_TARGET_SCOPE}
        sharedmodel
)

# DevKit Usage
if(SERVERBRIGE_COMPILE_SOURCES)
    if(DEBUG_BUILD AND COMMAND devkit_use_qt_shared)
        devkit_use_qt_shared(serverbrige Core)
    elseif(RELEASE_BUILD AND COMMAND devkit_use_qt_static)
        devkit_use_qt_static(serverbrige Core)
    endif()
endif()

if(SERVERBRIGE_COMPILE_SOURCES AND COMMAND devkit_finalize)
    devkit_finalize(serverbrige)
endif()

# Tests
if(UNITER_BUILD_TESTS)
    include(cmake/test.cmake)
endif()




