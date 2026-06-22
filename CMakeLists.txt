# Sources
include(cmake/sources.cmake)

# Target
add_library(messaging
    ${MESSAGING_SOURCES}
)

target_compile_features(messaging PUBLIC cxx_std_17)

# Angle Includes
include(cmake/include.cmake)

target_include_directories(messaging
    PUBLIC
        ${MESSAGING_INCLUDE}
)

# Linkage
target_link_libraries(messaging
    PUBLIC
        contract
)

# DevKit Usage
if(COMMAND devkit_finalize)
    devkit_finalize(messaging)
endif()

# Tests
if(UNITER_BUILD_TESTS)
    include(cmake/test.cmake)
endif()
