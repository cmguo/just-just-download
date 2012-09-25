// Manager.h
#ifndef _PPBOX_DOWNLOAD_MANAGER_H_
#define _PPBOX_DOWNLOAD_MANAGER_H_

#include "ppbox/download/Downloader.h"
#include "ppbox/download/CommonType.h"

#include <boost/thread.hpp>

namespace ppbox
{
    namespace download
    {
 
 		typedef void* DownloadHander;

		class Manager
            : public ppbox::common::CommonModuleBase<Manager>
        {
        public:
            Manager(
                util::daemon::Daemon & daemon);

            ~Manager();

        public:
            virtual boost::system::error_code startup();

            virtual void shutdown();

        public:
            Downloader * add(
                std::string const & playlink,
                std::string const & format,
                std::string const & filename,
                boost::system::error_code & ec,
			    Downloader::response_type const &  resp);

            boost::system::error_code del(
                Downloader * download_hander,
                boost::system::error_code & ec);

            boost::system::error_code get_download_statictis(
                Downloader * download_hander,
                DownloadStatistic & download_statistic,
                boost::system::error_code & ec);

            boost::system::error_code get_last_error(
                Downloader * downloader);

        private:
            void add_call_back(
                Downloader * downloader,
                boost::system::error_code const & ec);

        private:
            boost::mutex mutex_;
            std::vector<ppbox::download::DownloadInfo> info_vec_;
			boost::condition_variable cond_;
        };

    }//download
}//ppbox

#endif /* _PPBOX_DOWNLOAD_MANAGER_H_ */
