// Manager.h
#ifndef _PPBOX_DOWNLOAD_MANAGER_H_
#define _PPBOX_DOWNLOAD_MANAGER_H_

#include "ppbox/download/Downloader.h"
#include "ppbox/download/CommonType.h"

#include <vector>

#include <ppbox/common/CommonModuleBase.h>
#include <ppbox/demux/base/DemuxerType.h>
#ifndef  PPBOX_DISABLE_CERTIFY
#include <ppbox/certify/CertifyUserModule.h>
#endif

#include <boost/thread.hpp>

namespace ppbox
{
    namespace download
    {
 
 		typedef void* DownloadHander;

		class Manager
#ifdef  PPBOX_DISABLE_CERTIFY			
            : public ppbox::common::CommonModuleBase<Manager>
#else			
            : public ppbox::certify::CertifyUserModuleBase<Manager>
#endif            
        {
        public:
            // 进入认证成功状态
            virtual void certify_startup();

            // 退出认证成功状态
            virtual void certify_shutdown(
                boost::system::error_code const & ec);

            // 认证出错
            virtual void certify_failed(
                boost::system::error_code const & ec);

        public:
            Manager(
                util::daemon::Daemon & daemon);

            ~Manager();

            virtual boost::system::error_code startup();

            virtual void shutdown();

            Downloader * add(
                std::string const & playlink,
                std::string const & format,
                std::string const & filename,
                boost::system::error_code & ec,
			    Downloader::response_type const &  resp);

            boost::system::error_code del(
                Downloader * download_hander,
                boost::system::error_code & ec);

            void add_call_back(
                Downloader * downloader,
                boost::system::error_code const & ec);

            boost::system::error_code get_download_statictis(
                Downloader * download_hander,
                DownloadStatistic & download_statistic,
                boost::system::error_code & ec);

//            boost::uint32_t get_downloader_count(void) const;

            boost::system::error_code get_last_error(
                DownloadHander const download_hander) const;

            // 设置全局参数
            void set_download_path(
                char const * path);

        private:
            // config

            std::string storage_path_;
            boost::mutex mutex_;
            std::vector<ppbox::download::DownloadInfo> info_vec_;
			boost::condition_variable cond_;
        };

    }//download
}//ppbox

#endif /* _PPBOX_DOWNLOAD_MANAGER_H_ */
