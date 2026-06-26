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
if(SERVERBRIGE_COMPILE_SOURCES AND COMMAND devkit_finalize)
    devkit_finalize(serverbrige)
endif()

# Tests
if(UNITER_BUILD_TESTS)
    include(cmake/test.cmake)
endif()




