cmake_minimum_required(VERSION 3.12)

set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VESRION 0.0.${PATCH_VERSION})

project(bulk_server VERSION ${PROJECT_VESRION})

configure_file(version.h.in version.h)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

find_package(Boost)

add_executable(server
  command_processor.h
  output.h
  server.h
  server.cxx
  task_processor.h)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -Wall -pedantic")
set_target_properties(server PROPERTIES
                CXX_STANDARD 17
                CXX_STANDARD_REQUIRED ON
                COMPILE_OPTIONS "-O0;-Wall;"
)

option(CHECK_DOXYGEN "enable doc generation" ON)

if (${CHECK_DOXYGEN} STREQUAL ON)
    find_package(Doxygen)
    if (DOXYGEN_FOUND)
        set(DOXYFILE ${CMAKE_SOURCE_DIR}/Doxyfile)
        add_custom_target(doc_doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen")
        unset(DOXYFILE)
    endif()
endif()

target_link_libraries(server
    ${CMAKE_THREAD_LIBS_INIT}
)

target_include_directories(server
  PRIVATE "${CMAKE_BINARY_DIR}")

target_include_directories(server
  PRIVATE ${Boost_INCLUDE_DIR})

target_link_libraries(server
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/async/async.lib
  PRIVATE Boost::boost)

add_executable(client
  session.h
  session.cxx
  task_processor.h)

set_target_properties(client PROPERTIES
                CXX_STANDARD 17
                CXX_STANDARD_REQUIRED ON
                COMPILE_OPTIONS "-O0;-Wall;"
)

target_link_libraries(client
    ${CMAKE_THREAD_LIBS_INIT}
)

target_include_directories(client
  PRIVATE "${CMAKE_BINARY_DIR}")

target_include_directories(client
  PRIVATE ${Boost_INCLUDE_DIR})

target_link_libraries(client
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/async/async.lib
  PRIVATE Boost::boost)

install(TARGETS server RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_CONTACT example@example.com)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)
