# Versioning
project(netbridge VERSION 0.4.0 LANGUAGES CXX)

# Sources
include(cmake/sources.cmake)

# Target
if(NETBRIDGE_COMPILE_SOURCES)
    add_library(netbridge
        ${NETBRIDGE_SOURCES}
    )
    set(NETBRIDGE_TARGET_SCOPE PUBLIC)
else()
    add_library(netbridge INTERFACE)
    set(NETBRIDGE_TARGET_SCOPE INTERFACE)
endif()

target_compile_features(netbridge ${NETBRIDGE_TARGET_SCOPE} cxx_std_17)

# Angle Includes
include(cmake/include.cmake)

target_include_directories(netbridge
    ${NETBRIDGE_TARGET_SCOPE}
        ${NETBRIDGE_INCLUDE}
)

# Linkage
target_link_libraries(netbridge
    ${NETBRIDGE_TARGET_SCOPE}
        sharedmodel
)

# DevKit Usage
if(NETBRIDGE_COMPILE_SOURCES)
    if(DEBUG_BUILD AND COMMAND devkit_use_qt_shared)
        devkit_use_qt_shared(netbridge Core)
    elseif(RELEASE_BUILD AND COMMAND devkit_use_qt_static)
        devkit_use_qt_static(netbridge Core)
    endif()
endif()

if(NETBRIDGE_COMPILE_SOURCES AND COMMAND devkit_finalize)
    devkit_finalize(netbridge)
endif()

# Tests
if(UNITER_BUILD_TESTS)
    include(cmake/test.cmake)
endif()




