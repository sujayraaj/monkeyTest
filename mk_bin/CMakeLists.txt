set(src
  filesystem.c
  monkey.c
  mk_signals.c
  )

add_executable(monkey-bin ${src})
target_link_libraries(monkey-bin monkey-core-static)
set_target_properties(monkey-bin PROPERTIES OUTPUT_NAME monkey)

if(BUILD_LOCAL)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../${CMAKE_CURRENT_BINARY_DIR}/")
else()
  install(TARGETS monkey-bin RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_SBINDIR})
endif()
