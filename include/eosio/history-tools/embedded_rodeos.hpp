#pragma once

#include <eosio/history-tools/embedded_rodeos.h>
#include <new>
#include <stdexcept>

namespace embedded_rodeos {

struct error {
   rodeos_error* obj;

   error() : obj{ rodeos_create_error() } {
      if (!obj)
         throw std::bad_alloc{};
   }

   error(const error&) = delete;

   ~error() { rodeos_destroy_error(obj); }

   error& operator=(const error&) = delete;

   operator rodeos_error*() { return obj; }

   const char* msg() { return rodeos_get_error(obj); }

   template <typename F>
   auto check(F f) -> decltype(f()) {
      auto result = f();
      if (!result)
         throw std::runtime_error(msg());
      return result;
   }
};

struct context {
   struct error    error;
   rodeos_context* obj;

   context() : obj{ rodeos_create() } {
      if (!obj)
         throw std::bad_alloc{};
   }

   context(const context&) = delete;

   ~context() { rodeos_destroy(obj); }

   context& operator=(const context&) = delete;

   operator rodeos_context*() { return obj; }

   void open_db(const char* path, bool create_if_missing, int num_threads = 0, int max_open_files = 0) {
      error.check([&] { return rodeos_open_db(error, obj, path, create_if_missing, num_threads, max_open_files); });
   }
};

struct partition {
   struct error         error;
   rodeos_db_partition* obj;

   partition(rodeos_context* context, const char* prefix, uint32_t prefix_size) {
      obj = error.check([&] { return rodeos_create_partition(error, context, prefix, prefix_size); });
   }

   partition(const partition&) = delete;

   ~partition() { rodeos_destroy_partition(obj); }

   partition& operator=(const partition&) = delete;
};

struct snapshot {
   struct error        error;
   rodeos_db_snapshot* obj;

   snapshot(rodeos_db_partition* partition, bool persistent) {
      obj = error.check([&] { return rodeos_create_snapshot(error, partition, persistent); });
   }

   snapshot(const snapshot&) = delete;

   ~snapshot() { rodeos_destroy_snapshot(obj); }

   snapshot& operator=(const snapshot&) = delete;

   void refresh() {
      error.check([&] { return rodeos_refresh_snapshot(error, obj); });
   }

   void start_block(const char* data, uint64_t size) {
      error.check([&] { return rodeos_start_block(error, obj, data, size); });
   }

   void end_block(const char* data, uint64_t size, bool force_write) {
      error.check([&] { return rodeos_end_block(error, obj, data, size, force_write); });
   }

   template <typename F>
   void write_deltas(const char* data, uint64_t size, F shutdown) {
      error.check([&] {
         return rodeos_write_deltas(
               error, obj, data, size, [](void* f) -> rodeos_bool { return (static_cast<F*>(f))(); }, &shutdown);
      });
   }
};

struct filter {
   struct error   error;
   rodeos_filter* obj;

   filter(const char* wasm_filename) {
      obj = error.check([&] { return rodeos_create_filter(error, wasm_filename); });
   }

   filter(const filter&) = delete;

   ~filter() { rodeos_destroy_filter(obj); }

   filter& operator=(const filter&) = delete;

   void run(rodeos_db_snapshot* snapshot, const char* data, uint64_t size) {
      error.check([&] { return rodeos_run_filter(error, snapshot, obj, data, size); });
   }
};

} // namespace embedded_rodeos