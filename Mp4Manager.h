// Manager.h
#ifndef _PPBOX_MP4_DOWNLOAD_MANAGER_H_
#define _PPBOX_MP4_DOWNLOAD_MANAGER_H_



#include <ppbox/common/CommonModuleBase.h>
#include <ppbox/demux/DemuxerType.h>

#ifndef  PPBOX_DISABLE_CERTIFY
#include <ppbox/certify/CertifyUserModule.h>
#endif

#include <vector>

namespace ppbox
{
    namespace download
    {

        class Mp4Manager
            : public ppbox::common::CommonModuleBase<Mp4Manager>
        {
        public:

            Mp4Manager(util::daemon::Daemon & daemon);
            ~Mp4Manager();

            virtual boost::system::error_code startup();

            virtual void shutdown();

            boost::system::error_code add(char const * playlink,
                                          char const * format,
                                          char const * dest,
                                          char const * filename,
                                          void* & download_hander,
                                          boost::system::error_code & ec);

            boost::system::error_code del(void* const download_hander,
                                          boost::system::error_code & ec);

            boost::system::error_code get_download_statictis(void* const download_hander,
                                                             FileDownloadStatistic & download_statistic,
                                                             boost::system::error_code & ec);

            boost::uint32_t get_downloader_count(void) const;

            boost::system::error_code get_last_error(void* const download_hander) const;

            // 设置全局参数
            void set_download_path(char const * path);

        private:
             bool find(void* const download_hander) const;

        private:
            ppbox::certify::Certifier& ceri_;
            boost::asio::io_service & io_srv_;
            // config
            std::string               storage_path_;

            boost::int32_t count_;
        };
    }
}

#endif /* _PPBOX_DOWNLOAD_MANAGER_H_ */
