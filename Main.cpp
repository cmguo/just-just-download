// HttpProxy.cpp

#include "ppbox/download/Common.h"
#include "ppbox/download/Version.h"
#include "ppbox/download/Manager.h"

//#include <framework/logger/LoggerSection.h>
#include <framework/process/Process.h>
#include <framework/process/SignalHandler.h>

#include <boost/bind.hpp>

FRAMEWORK_LOGGER_DECLARE_MODULE("ppbox_download");

int download_main(int argc, char * argv[])
{
    util::daemon::Daemon my_daemon("ppbox_download.conf");
    char const * default_argv[] = {
        "++Logger.stream_count=1", 
        "++Logger.ResolverService=1", 
        "++LogStream0.file=$LOG/ppbox_download.log", 
        "++LogStream0.append=true", 
        "++LogStream0.roll=true", 
        "++LogStream0.level=5", 
        "++LogStream0.size=102400", 
    };
    my_daemon.parse_cmdline(sizeof(default_argv) / sizeof(default_argv[0]), default_argv);
    my_daemon.parse_cmdline(argc, (char const **)argv);

    framework::process::SignalHandler sig_infor(
        framework::process::Signal::sig_int, 
        boost::bind(&util::daemon::Daemon::post_stop, &my_daemon), true);

    framework::logger::global_logger().load_config(my_daemon.config());

    ppbox::common::log_versions();

    ppbox::common::CommonModule & common = 
        util::daemon::use_module<ppbox::common::CommonModule>(my_daemon, "ppbox_download");
    common.set_version(ppbox::download::version());


    util::daemon::use_module<ppbox::download::Manager>(my_daemon);

    my_daemon.start(framework::this_process::notify_wait);


    return 0;
}

#ifndef _LIB
int main(int argc, char * argv[])
{
    return download_main( argc, argv);
}
#endif

