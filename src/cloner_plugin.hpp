// copyright defined in LICENSE.txt

#pragma once
#include "rocksdb_plugin.hpp"
#include "streamer_plugin.hpp"

class cloner_plugin : public appbase::plugin<cloner_plugin> {
 public:
   APPBASE_PLUGIN_REQUIRES((rocksdb_plugin)(streamer_plugin))

   cloner_plugin();
   virtual ~cloner_plugin();

   virtual void set_program_options(appbase::options_description& cli, appbase::options_description& cfg) override;
   void         plugin_initialize(const appbase::variables_map& options);
   void         plugin_startup();
   void         plugin_shutdown();

 private:
   std::shared_ptr<struct cloner_plugin_impl> my;
};
