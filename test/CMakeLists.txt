function(sched_add_test name srcs)
  add_executable(${name} ${srcs})
  target_link_libraries(${name} PRIVATE sched)
  target_compile_options(${name} PRIVATE ${WARNING_FLAGS})
  target_compile_features(${name} PRIVATE c_std_11)
  add_test(NAME ${name} COMMAND ${name})

  set(TMPDIR "${CMAKE_CURRENT_BINARY_DIR}/${name}.tmp")
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

sched_add_test(test_sched sched.c)
