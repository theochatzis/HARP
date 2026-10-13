# Helper macros that make adding a package a one-liner.
#
#   harp_add_library(Name SOURCES ... DEPENDS ...)   -> lib/libHarp<Name>.so, headers from <pkg>/interface
#   harp_add_plugin(Name SOURCES ... DEPENDS ...)    -> lib/plugins/libHarp<Name>Plugin.so (dlopen'ed by harpRun/harpPlot)
#   harp_add_executable(name SOURCES ... DEPENDS ...)-> bin/<name>

function(harp_add_library NAME)
  cmake_parse_arguments(ARG "" "" "SOURCES;DEPENDS" ${ARGN})
  set(target Harp${NAME})
  add_library(${target} SHARED ${ARG_SOURCES})
  target_include_directories(${target} PUBLIC ${HARP_SOURCE_DIR})
  target_link_libraries(${target} PUBLIC ${ARG_DEPENDS} ${HARP_ROOT_LIBS} nlohmann_json::nlohmann_json)
  target_compile_options(${target} PRIVATE ${HARP_WARNINGS})
  set_target_properties(${target} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${HARP_BINARY_DIR}/lib
    OUTPUT_NAME Harp${NAME})
endfunction()

function(harp_add_plugin NAME)
  cmake_parse_arguments(ARG "" "" "SOURCES;DEPENDS" ${ARGN})
  set(target Harp${NAME}Plugin)
  add_library(${target} MODULE ${ARG_SOURCES})
  target_include_directories(${target} PRIVATE ${HARP_SOURCE_DIR})
  target_link_libraries(${target} PRIVATE HarpCore ${ARG_DEPENDS} ${HARP_ROOT_LIBS} nlohmann_json::nlohmann_json)
  target_compile_options(${target} PRIVATE ${HARP_WARNINGS})
  set_target_properties(${target} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${HARP_BINARY_DIR}/lib/plugins
    PREFIX "lib"
    OUTPUT_NAME Harp${NAME}Plugin)
endfunction()

function(harp_add_executable NAME)
  cmake_parse_arguments(ARG "" "" "SOURCES;DEPENDS" ${ARGN})
  add_executable(${NAME} ${ARG_SOURCES})
  target_include_directories(${NAME} PRIVATE ${HARP_SOURCE_DIR})
  target_link_libraries(${NAME} PRIVATE HarpCore ${ARG_DEPENDS} ${HARP_ROOT_LIBS} nlohmann_json::nlohmann_json ${CMAKE_DL_LIBS})
  target_compile_options(${NAME} PRIVATE ${HARP_WARNINGS})
  set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${HARP_BINARY_DIR}/bin)
endfunction()
