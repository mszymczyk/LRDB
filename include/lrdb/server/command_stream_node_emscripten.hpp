#pragma once

#include <emscripten.h>
#include <unistd.h>
#include <cstdint>

extern "C" void lrdb_server_socket_on_connect(void* ptr);
extern "C" void lrdb_server_socket_on_close(void* ptr);
extern "C" void lrdb_server_socket_on_error(void* ptr, const char* msg);
extern "C" void lrdb_server_socket_on_data(void* ptr, const char* data);

namespace lrdb {
// one to one server socket
class command_stream_socket {
 public:
  command_stream_socket() {
    EM_ASM_(
        {
          var this_ptr = $0;
          process.on('message', function(data) {
            var result = Module.ccall(
                'lrdb_server_socket_on_data',  // name of C function
                'void',                        // return type
                [ 'number', "string" ],        // argument types
                [ this_ptr, JSON.stringify(data) ]);
          });
          process.on('exit', function() {
            var result = Module.ccall(
                'lrdb_server_socket_on_close',  // name of C function
                'void',                         // return type
                ['number'],                     // argument types
                [this_ptr]);
          });
        },
        this);
  }
  ~command_stream_socket() {}
  void close() {
    EM_ASM({
      process.on('message', function(data){});
      process.on('exit', function(){});
    });
  }

  std::function<void(const std::string& data)> on_data;
  std::function<void()> on_connection;
  std::function<void()> on_close;
  std::function<void(const std::string&)> on_error;

  bool is_open() const { return true; }
  void poll() {
    static int rate = 0;
    if (rate % 1000 == 0) {
      emscripten_sleep(1);
    }
    rate++;
  }
  void run_one() { emscripten_sleep(1); }
  void wait_for_connection() {}

  bool send_message(const std::string& m) {
    return EM_ASM_INT(
        {
          var data = Pointer_stringify($0);
          process.send(JSON.parse(data));
          return 1;
        },
        m.c_str());
    return true;
  }

  // private
  void on_con_() {
    if (on_connection) {
      on_connection();
    }
  }
  void on_clo_() {
    if (on_close) {
      on_close();
    }
  }
  void on_err_(const char* msg) {
    if (on_error) {
      on_error(msg);
    }
  }
  void on_dat_(const char* data) {
    if (on_data) {
      on_data(data);
    }
  }

 private:
};
}

extern "C" void lrdb_server_socket_on_connect(void* ptr) {
  lrdb::command_stream_socket* this_ =
      static_cast<lrdb::command_stream_socket*>(ptr);
  this_->on_con_();
}
extern "C" void lrdb_server_socket_on_close(void* ptr) {
  lrdb::command_stream_socket* this_ =
      static_cast<lrdb::command_stream_socket*>(ptr);
  this_->on_clo_();
}
extern "C" void lrdb_server_socket_on_error(void* ptr, const char* msg) {
  lrdb::command_stream_socket* this_ =
      static_cast<lrdb::command_stream_socket*>(ptr);
  this_->on_err_(msg);
}
extern "C" void lrdb_server_socket_on_data(void* ptr, const char* data) {
  lrdb::command_stream_socket* this_ =
      static_cast<lrdb::command_stream_socket*>(ptr);
  this_->on_dat_(data);
}
