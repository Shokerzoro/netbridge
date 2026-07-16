file(GLOB_RECURSE NETBRIDGE_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
)

if(NETBRIDGE_TEST_SOURCES)
    add_executable(netbridge_tests
        ${NETBRIDGE_TEST_SOURCES}
    )

    target_link_libraries(netbridge_tests
        PRIVATE
            GTest::gtest_main
            netbridge
    )

    uniter_register_gtest(netbridge_tests)
endif()
