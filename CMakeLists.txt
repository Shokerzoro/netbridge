# Versioning
project(tenantbridge VERSION 0.2.0 LANGUAGES CXX)

# Sources
include(cmake/sources.cmake)

# Target
if(TENANTBRIDGE_COMPILE_SOURCES)
    add_library(tenantbridge
        ${TENANTBRIDGE_SOURCES}
    )
    set(TENANTBRIDGE_TARGET_SCOPE PUBLIC)
else()
    add_library(tenantbridge INTERFACE)
    set(TENANTBRIDGE_TARGET_SCOPE INTERFACE)
endif()

target_compile_features(tenantbridge ${TENANTBRIDGE_TARGET_SCOPE} cxx_std_17)

# Angle Includes
include(cmake/include.cmake)

target_include_directories(tenantbridge
    ${TENANTBRIDGE_TARGET_SCOPE}
        ${TENANTBRIDGE_INCLUDE}
)

# Linkage
target_link_libraries(tenantbridge
    ${TENANTBRIDGE_TARGET_SCOPE}
        sharedmodel
)

# DevKit Usage
if(TENANTBRIDGE_COMPILE_SOURCES)
    if(DEBUG_BUILD AND COMMAND devkit_use_qt_shared)
        devkit_use_qt_shared(tenantbridge Core)
    elseif(RELEASE_BUILD AND COMMAND devkit_use_qt_static)
        devkit_use_qt_static(tenantbridge Core)
    endif()
endif()

if(TENANTBRIDGE_COMPILE_SOURCES AND COMMAND devkit_finalize)
    devkit_finalize(tenantbridge)
endif()

# Tests
if(UNITER_BUILD_TESTS)
    include(cmake/test.cmake)
endif()




