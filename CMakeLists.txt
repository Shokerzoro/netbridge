# Sources
include(cmake/sources.cmake)

# Target
if(MESSAGING_COMPILE_SOURCES)
    add_library(messaging
        ${MESSAGING_SOURCES}
    )
    set(MESSAGING_TARGET_SCOPE PUBLIC)
else()
    add_library(messaging INTERFACE)
    set(MESSAGING_TARGET_SCOPE INTERFACE)
endif()

target_compile_features(messaging ${MESSAGING_TARGET_SCOPE} cxx_std_17)

# Angle Includes
include(cmake/include.cmake)

target_include_directories(messaging
    ${MESSAGING_TARGET_SCOPE}
        ${MESSAGING_INCLUDE}
)

# Linkage
target_link_libraries(messaging
    ${MESSAGING_TARGET_SCOPE}
        contract
)

# DevKit Usage
if(MESSAGING_COMPILE_SOURCES AND COMMAND devkit_finalize)
    devkit_finalize(messaging)
endif()

# Tests
if(UNITER_BUILD_TESTS)
    include(cmake/test.cmake)
endif()




