// Manager.h
#ifndef _PPBOX_DOWNLOAD_MANAGER_H_
#define _PPBOX_DOWNLOAD_MANAGER_H_
#include "ppbox/download/Downloader.h"

#include <vector>

#include <ppbox/common/CommonModuleBase.h>
#include <ppbox/demux/base/DemuxerType.h>
#ifndef  PPBOX_DISABLE_CERTIFY
#include <ppbox/certify/CertifyUserModule.h>
#endif

namespace ppbox
{
    namespace download
    {
        typedef void * DownloadHander;

        struct DownloadInsideHander
        {
            DownloadInsideHander & operator=(DownloadInsideHander const & download_hander)
            {
                this->playlink = download_hander.playlink;
                this->format   = download_hander.format;
                this->dest     = download_hander.dest;
                this->downloder= download_hander.downloder;
                return *this;
            }

            std::string           playlink;
            std::string           format;
            std::string           dest;
            Downloader::pointer   downloder;
        };

        typedef std::vector<DownloadInsideHander *>       DownloaderList;

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
             bool find(DownloadHander const download_hander) const;

        private:
            boost::asio::io_service & io_srv_;
            DownloaderList            downloader_list_;

            // config
            std::string               storage_path_;
        };
    }
}

#endif /* _PPBOX_DOWNLOAD_MANAGER_H_ */
