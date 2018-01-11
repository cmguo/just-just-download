// DownloadModule.h

#ifndef _JUST_DOWNLOAD_DOWNLOAD_MODULE_H_
#define _JUST_DOWNLOAD_DOWNLOAD_MODULE_H_

#include <framework/string/Url.h>

#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>

namespace just
{
    namespace download
    {

        class Downloader;

        class DownloadModule
            : public just::common::CommonModuleBase<DownloadModule>
        {
        public:
            typedef boost::function<void (
                boost::system::error_code const &, 
                Downloader *)
            > open_response_type;

        public:
            DownloadModule(
                util::daemon::Daemon & daemon);

            ~DownloadModule();

        public:
            virtual bool startup(
                boost::system::error_code & ec);

            virtual bool shutdown(
                boost::system::error_code & ec);

        public:
            Downloader * create();

            Downloader * open(
                framework::string::Url const & url, 
                open_response_type const & resp);

            void open(
                Downloader * downloader,
                framework::string::Url const & url, 
                open_response_type const & resp);

            void start(
                Downloader * downloader, long start, long end,
                open_response_type const & resp);
            
            bool close(
                Downloader * downloader, 
                boost::system::error_code & ec);

            bool cancel(
                Downloader * downloader, 
                boost::system::error_code & ec);
                
            Downloader * find(
                framework::string::Url const & url);

        private:
            struct DownloadInfo;

        private:
            DownloadInfo * create(
                framework::string::Url const & url, 
                open_response_type const & resp, 
                boost::system::error_code & ec);

            void async_open(
                DownloadInfo * info);

            void async_start(
                Downloader * downloader, long start, long end);

            void handle_open(
                boost::system::error_code const & ec,
                DownloadInfo * info);
            
            void handle_start(
                boost::system::error_code const & ecc,
                Downloader * downloader);

            boost::system::error_code close_locked(
                DownloadInfo * info, 
                bool inner_call, 
                boost::system::error_code & ec);

            boost::system::error_code close(
                DownloadInfo * info, 
                boost::system::error_code & ec);

            boost::system::error_code cancel(
                DownloadInfo * info, 
                boost::system::error_code & ec);

            void destory(
                DownloadInfo * info);
            
            void dump_info(DownloadInfo * info,
                        char const * func, 
                        int line);

        private:
            std::vector<DownloadInfo *> demuxers_;
            boost::mutex mutex_;
        };

    } // namespace download
} // namespace just

#endif // _JUST_DOWNLOAD_DOWNLOAD_MODULE_H_
