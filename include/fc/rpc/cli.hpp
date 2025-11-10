#pragma once
#include <fc/io/stdio.hpp>
#include <fc/io/json.hpp>
#include <fc/io/buffered_iostream.hpp>
#include <fc/io/sstream.hpp>
#include <fc/rpc/api_connection.hpp>
#include <fc/thread/thread.hpp>

#include <iostream>
#include <optional>
#include <memory>

#include <memory>
#include <stdexcept>
#include <utility>

#include <fc/stdcomp.hpp>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

namespace fc { namespace rpc {

   /**
    *  A funciton that will will give commands to run in the CLI, e.g. reading from pipe or other.
    */
   class cli_cmd_provider {
      public:
         virtual ~cli_cmd_provider()=default;
         virtual std::string get_name()=0;
         virtual std::string get_short_info()=0;
         virtual std::string read_command()=0;
   };

   class cli_cmd_provider_pipe : public cli_cmd_provider {
      public:
         static constexpr long int max_cmd_len = 1024*1024 * 16;
      protected:
         const int m_info_fd_commands, m_info_fd_response; ///< keep the FD numbers we got, just for information (do not directly use them for read/write)

         using fd_source = boost::iostreams::file_descriptor_source;
         using fd_sink = boost::iostreams::file_descriptor_sink;
         using fd_stream_in = boost::iostreams::stream<fd_source>;
         using fd_stream_out = boost::iostreams::stream<fd_sink>;    
         std::unique_ptr<fd_stream_in> cmd_in_file;   // command input pipe - as boost iostream
         std::unique_ptr<fd_stream_out> cmd_out_file; // command output pipe - as boost iostream

      public:
         cli_cmd_provider_pipe(int fd_commands, int fd_response);
         virtual ~cli_cmd_provider_pipe()=default;
         virtual std::string get_name();
         virtual std::string get_short_info();
         virtual std::string read_command();
   };

   /**
    *  Provides a simple wrapper for RPC calls to a given interface.
    */
   class cli : public api_connection
   {
      public:
         cli( uint32_t max_depth ) : api_connection(max_depth) {}
         virtual ~cli();

         virtual variant send_call( api_id_type api_id, string method_name, variants args = variants() );
         virtual variant send_callback( uint64_t callback_id, variants args = variants() );
         virtual void    send_notice( uint64_t callback_id, variants args = variants() );

         using t_cmd_provider = std::function<std::string(void)>; ///< functions that provide commands
         fc::stdcomp::optional< std::weak_ptr< t_cmd_provider> > m_cmd_provider;

         virtual void set_read_hook(std::weak_ptr<t_cmd_provider> provider);

         void start();
         void stop();
         void cancel();
         void wait();
         void format_result( const string& method, std::function<string(variant,const variants&)> formatter);

         virtual void getline( const std::string& prompt, std::string& line );

         void set_prompt( const string& prompt );

         void set_regex_secret( const string& expr );

      private:
         void run();

         std::string _prompt = ">>>";
         std::map<string,std::function<string(variant,const variants&)> > _result_formatters;
         fc::future<void> _run_complete;
         fc::thread* _getline_thread = nullptr; ///< Wait for user input in this thread
   };
} } 
