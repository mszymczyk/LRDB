#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif
#include <iostream>

#ifdef LRDB_ENABLE_STDINOUT_STREAM
#include "lrdb/command_stream_stdstream.hpp"
#endif
#include "lrdb/server.hpp"

template <typename DebugServer>
int exec(const char* program, DebugServer& debug_server, int argc,
         char* argv[]) {
  lua_State* L = luaL_newstate();
  luaL_openlibs(L);

  debug_server.reset(L);
  int ret = luaL_loadfile(L, program);

  if (ret == 0) {
    for (int i = 0; i < argc; ++i) {
      lua_pushstring(L, argv[i]);
    }
    ret = lua_pcall(L, argc, LUA_MULTRET, 0);
  }
  if (ret != 0) {
    std::cerr << lua_tostring(L, -1);  // output error
  }
  debug_server.reset();

  lua_close(L);
  L = 0;
#ifdef EMSCRIPTEN
  emscripten_force_exit(ret ? 1 : 0);
#endif
  return ret ? 1 : 0;
}

int main(int argc, char* argv[]) {
  int port = 0;
  const char* program = 0;

  // parse args
  int i = 1;
  for (; i < argc; ++i) {
    if (argv[i][0] == '-') {
      if (i + 1 < argc) {
        if (strcmp(argv[i], "-p") || strcmp(argv[i], "--port")) {
          port = atoi(argv[i + 1]);
          ++i;
        }
      } else {
        return 1;  // invalid argument
      }
    } else {
      program = argv[i];
      ++i;
      break;
    }
  }
  if (!program) {
    return 1;
  }
  std::cout << LUA_COPYRIGHT << std::endl;
#ifdef EMSCRIPTEN
  EM_ASM_(
      {
        var path = require('path');
        var program_path = Pointer_stringify($0);
        var program_full_path = path.resolve(program_path);
        var token = path.parse(program_full_path);
        var cwd = path.relative(token.root, process.cwd());
        FS.mkdir('root');
        FS.mount(NODEFS, {root : token.root}, 'root');
        FS.chdir('root/' + cwd);
      },
      program);
#endif

#ifdef LRDB_USE_NODE_CHILD_PROCESS
  lrdb::server debug_server;
  return exec(program, debug_server, argc - i, &argv[i]);
#else
  if (port == 0)  // if no port use std::cin and std::cout
  {
#ifdef LRDB_ENABLE_STDINOUT_STREAM
    lrdb::basic_server<lrdb::command_stream_stdstream> debug_server(std::cin,
                                                                    std::cout);
    return exec(program, debug_server, argc - i, &argv[i]);
#else
    return -1;
#endif
  } else {
    lrdb::server debug_server(port);
    return exec(program, debug_server, argc - i, &argv[i]);
  }
#endif
}