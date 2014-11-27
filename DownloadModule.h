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
            virtual boost::system::error_code startup();

            virtual void shutdown();

        public:
            Downloader * open(
                framework::string::Url const & url, 
                open_response_type const & resp);

            bool close(
                Downloader * downloader, 
                boost::system::error_code & ec);

            Downloader * find(
                framework::string::Url const & url);

        private:
            struct DemuxInfo;

        private:
            DemuxInfo * create(
                framework::string::Url const & url, 
                open_response_type const & resp, 
                boost::system::error_code & ec);

            void async_open(
                boost::mutex::scoped_lock & lock, 
                DemuxInfo * info);

            void handle_open(
                boost::system::error_code const & ec,
                DemuxInfo * info);

            boost::system::error_code close_locked(
                DemuxInfo * info, 
                bool inner_call, 
                boost::system::error_code & ec);

            boost::system::error_code close(
                DemuxInfo * info, 
                boost::system::error_code & ec);

            boost::system::error_code cancel(
                DemuxInfo * info, 
                boost::system::error_code & ec);

            void destory(
                DemuxInfo * info);

        private:
            std::vector<DemuxInfo *> demuxers_;
            boost::mutex mutex_;
        };

    } // namespace download
} // namespace just

#endif // _JUST_DOWNLOAD_DOWNLOAD_MODULE_H_
