// Manager.h
#ifndef _PPBOX_DOWNLOAD_MANAGER_H_
#define _PPBOX_DOWNLOAD_MANAGER_H_


#include <vector>

#include <ppbox/common/CommonModuleBase.h>
#include <ppbox/demux/base/DemuxerType.h>
#ifndef  PPBOX_DISABLE_CERTIFY
#include <ppbox/certify/CertifyUserModule.h>
#endif


#include <boost/asio/deadline_timer.hpp>

namespace ppbox
{
    namespace download
    {

        class Mp4Manager;
        class FlvTsManager;

        struct DownloadStatistic
        {
            DownloadStatistic()
            {
                total_size = 0;
                finish_size = 0;
                speed = 0;
            }
			boost::uint64_t total_size;  
            boost::uint64_t finish_size;
            boost::uint32_t speed; // B/s
        };

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
            Manager(util::daemon::Daemon & daemon);
            ~Manager();

            virtual boost::system::error_code startup();

            virtual void shutdown();

            boost::system::error_code add(char const * playlink,
                                          char const * format,
                                          char const * dest,
                                          char const * filename,
                                          DownloadHander & download_hander,
                                          boost::system::error_code & ec);

            boost::system::error_code del(DownloadHander const download_hander,
                                          boost::system::error_code & ec);

            boost::system::error_code get_download_statictis(DownloadHander const download_hander,
                                                             DownloadStatistic & download_statistic,
                                                             boost::system::error_code & ec);

            boost::uint32_t get_downloader_count(void) const;

            boost::system::error_code get_last_error(DownloadHander const download_hander) const;

            // 设置全局参数
            void set_download_path(char const * path);

        private:
            // config

            boost::asio::io_service & io_srv_;
            Mp4Manager& mp4_module_;
            FlvTsManager& flvts_module_;
            std::string               storage_path_;
        };
    }
}

#endif /* _PPBOX_DOWNLOAD_MANAGER_H_ */
