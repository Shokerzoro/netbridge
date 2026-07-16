file(GLOB_RECURSE TENANTBRIDGE_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
)

if(TENANTBRIDGE_TEST_SOURCES)
    add_executable(tenantbridge_tests
        ${TENANTBRIDGE_TEST_SOURCES}
    )

    target_link_libraries(tenantbridge_tests
        PRIVATE
            GTest::gtest_main
            tenantbridge
    )

    uniter_register_gtest(tenantbridge_tests)
endif()
