set(testdir "${CMAKE_CURRENT_BINARY_DIR}")

set(SRC ${CMAKE_CURRENT_SOURCE_DIR})
file(CREATE_LINK "${SRC}/prod.tsv" "${testdir}/prod.tsv" COPY_ON_ERROR)
add_compile_definitions(TESTDIR="${testdir}")

function(sched_add_test name srcs)
  add_executable(${name} ${srcs})
  target_link_libraries(${name} PRIVATE sched)
  target_compile_options(${name} PRIVATE ${WARNING_FLAGS})
  target_compile_features(${name} PRIVATE c_std_11)
  add_test(NAME ${name} COMMAND ${name})

  set(TMPDIR "${testdir}/${name}.tmp")
  add_custom_command(
    TARGET ${name}
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TMPDIR})
  add_custom_command(
    TARGET ${name}
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TMPDIR}/dir)
  target_compile_definitions(${name} PUBLIC "TMPDIR=\"${TMPDIR}\"")
endfunction()

sched_add_test(test_sched "sched.c;fs.c")
