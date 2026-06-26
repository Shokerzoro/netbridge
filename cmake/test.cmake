file(GLOB_RECURSE SERVERBRIGE_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
)

if(SERVERBRIGE_TEST_SOURCES)
    add_executable(serverbrige_tests
        ${SERVERBRIGE_TEST_SOURCES}
    )

    target_link_libraries(serverbrige_tests
        PRIVATE
            GTest::gtest_main
            serverbrige
    )

    uniter_register_gtest(serverbrige_tests)
endif()
