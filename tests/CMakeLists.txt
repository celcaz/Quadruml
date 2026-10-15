find_package(Qt6 REQUIRED COMPONENTS Test)

# Os testes precisam achar as DLLs do Qt ao rodar pelo ctest.
get_filename_component(QUADRUM_QT_BIN "${Qt6_DIR}/../../../bin" ABSOLUTE)

function(quadrum_add_test name)
    qt_add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE quadrum_core Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${QUADRUM_QT_BIN}"
    )
endfunction()

quadrum_add_test(tst_database)
quadrum_add_test(tst_profilestore)
quadrum_add_test(tst_disc)
quadrum_add_test(tst_gamerepository)
quadrum_add_test(tst_steam)
quadrum_add_test(tst_metadata)
