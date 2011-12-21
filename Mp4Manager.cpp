// Mp4Manager.cpp
#include "ppbox/download/Common.h"
#include "ppbox/download/Mp4Manager.h"
#include "ppbox/download/Mp4Dispatcher.h"

#include <ppbox/certify/Certifier.h>

#include <util/protocol/pptv/Url.h>
#include <utility>
using namespace boost::system;

#include <framework/string/StringToken.h>

#include <framework/logger/LoggerStreamRecord.h>
using namespace framework::logger;

FRAMEWORK_LOGGER_DECLARE_MODULE("Mp4Manager");

namespace ppbox
{
    namespace download
    {
        Mp4Manager::Mp4Manager(util::daemon::Daemon & daemon)
			: ppbox::common::CommonModuleBase<Mp4Manager>(daemon, "Mp4Manager")
            , ceri_(util::daemon::use_module<ppbox::certify::Certifier>(daemon))
            , io_srv_(io_svc())
            ,count_(0)
        {
        }

        Mp4Manager::~Mp4Manager()
        {
        }

        error_code Mp4Manager::startup()
        {
            return error_code();
        }

        void Mp4Manager::shutdown()
        {
        }

        void callback(error_code const & ec)
        {
            std::cout<<"async_open callback ec:"<<ec.message()<<std::endl;
        }

        error_code Mp4Manager::add(char const * playlink,
                                char const * format,
                                char const * dest,
                                char const * filename,
                                void* & download_hander,
                                error_code & ec)
        {
            //хож╓
            Mp4Dispatcher* pMp4 = new Mp4Dispatcher(get_daemon());
            ec = pMp4->add(playlink,format,dest,filename);
            download_hander = (void*)pMp4;
            ++count_;
            return ec;
        }

        error_code Mp4Manager::del(void* const download_hander,
                                error_code & ec)
        {
           ((Mp4Dispatcher*)download_hander)->del(ec);
           delete (Mp4Dispatcher*)download_hander;
           --count_;
            return ec;
        }

        error_code Mp4Manager::get_download_statictis(void* const download_hander,
                                                   FileDownloadStatistic & download_statistic,
                                                   error_code & ec)
        {
           ((Mp4Dispatcher*)download_hander)->get_download_statictis(download_statistic,ec);
            return ec;
        }

        boost::uint32_t Mp4Manager::get_downloader_count(void) const
        {
            return count_;
        }

        error_code Mp4Manager::get_last_error(void* const download_hander) const
        {
            error_code ec;
            return ec;

        }

        void Mp4Manager::set_download_path(char const * path)
        {
            storage_path_ = path;
        }

        bool Mp4Manager::find(void* const download_hander) const
        {
            bool res = false;
            return res;
        }

    }
}
