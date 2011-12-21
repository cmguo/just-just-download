// Manager.h
#ifndef _PPBOX_FLVTS_DOWNLOAD_MANAGER_H_
#define _PPBOX_FLVTS_DOWNLOAD_MANAGER_H_

#include <ppbox/common/CommonModuleBase.h>
#ifndef  PPBOX_DISABLE_CERTIFY
#include <ppbox/certify/CertifyUserModule.h>
#endif

namespace ppbox
{
    namespace download
    {
        struct FileDownloadStatistic;
        class DownloadDispatcher;

        class FlvTsManager
            : public ppbox::common::CommonModuleBase<FlvTsManager>
        
        {
        public:
            FlvTsManager(util::daemon::Daemon & daemon);
            ~FlvTsManager();

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

        private:
            DownloadDispatcher* get_freeDispatch();
        private:
            std::vector<DownloadDispatcher*> dispatcher_; //线程列表
        };
    }
}

#endif /* _PPBOX_DOWNLOAD_MANAGER_H_ */
