# Every sub-directory with a CMakeLists.txt is a plugin package.
# To add a package: mkdir Modules/MyPkg/plugins, add Modules/MyPkg/CMakeLists.txt with
#   harp_add_plugin(MyPkg SOURCES plugins/A.cc plugins/B.cc DEPENDS HarpDataFormats)
file(GLOB pkgs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
foreach(pkg ${pkgs})
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${pkg}/CMakeLists.txt)
    add_subdirectory(${pkg})
  endif()
endforeach()
