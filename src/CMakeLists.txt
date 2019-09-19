include (FindPkgConfig)
include (FindBoost)

find_package (Threads REQUIRED)
find_package (Boost REQUIRED COMPONENTS "system" "filesystem" "program_options")
pkg_search_module (LIBGIT2 REQUIRED libgit2)

set (Boost_USE_STATIC_LIBS ON)

include_directories (${Boost_INCLUDE_DIRS})
link_directories (${Boost_LIBRARY_DIRS})
add_definitions (${Boost_LIB_DIAGNOSTIC_DEFINITIONS})

include_directories (${LIBGIT2_INCLUDE_DIRS})
link_directories (${LIBGIT2_LIBRARY_DIRS})
add_definitions (${LIBGIT2_LIB_DIAGNOSTIC_DEFINITIONS})

set (SOURCE_FILES
  main.cc;
  args.h;
  args.cc;
  timer.h;
  timer.cc;
  discord.h;
  discord.cc;
  git_walker.h;
  git_walker.cc;
  git_hook.h;
  git_hook.cc;
  session.h;
  session.cc;
  git.h;
  git.cc;
  utils.h;
  utils.cc;
  server.h;
  server.cc;
  language.h;
  language.cc;
  client.h;
  client.cc;
  client_socket.h;
  client_socket.cc;
  connection.h;
  connection.cc;
  async_replace.h;
  async_replace.cc;
  discord_presence.h;
  editor_icon.h
  )

add_executable (presence ${SOURCE_FILES})

target_include_directories (presence PRIVATE ${CMAKE_INCLUDE_PATH})
target_include_directories (presence PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries (presence
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  ${LIBGIT2_LIBRARIES}
  )

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_link_libraries( presence
    wsock32
    ws2_32
    )
endif ()

install (TARGETS presence DESTINATION bin)
