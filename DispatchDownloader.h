// DispatchDownloader.h

#ifndef _JUST_DOWNLOAD_DISPATCH_DOWNLOADHER_H_
#define _JUST_DOWNLOAD_DISPATCH_DOWNLOADHER_H_

#include "just/download/Downloader.h"

#include <just/dispatch/DispatchBase.h>

#include <util/stream/UrlSink.h>
#include <util/stream/UrlSource.h>

namespace just
{
    namespace data
    {
        class UrlSink;
    }

    namespace download
    { 

        class DispatchDownloader 
            : public Downloader
        {
        public:
            DispatchDownloader(
                boost::asio::io_service & io_svc);

            ~DispatchDownloader();

        public:
            virtual void open(
                framework::string::Url const & url,
                response_type const & resp);

            virtual bool cancel(
                boost::system::error_code & ec);

            virtual bool close(
                boost::system::error_code & ec);

            virtual bool get_statictis(
                DownloadStatistic & stat,
                boost::system::error_code & ec);

            virtual void start(
                long start, long end,
                response_type const & resp);

        private:
            //void handle_source_open(
            //    boost::system::error_code const & ec);

            //void handle_source_read(
            //    boost::system::error_code const & ec);

            void handle_sink_open(
                boost::system::error_code const & ec);

            void handle_dispatcher_open(
                boost::system::error_code ec);

            void handle_play(
                boost::system::error_code const & ec);
            
        private:
            just::dispatch::DispatcherBase * dispatcher_;
            framework::string::Url url_;
            //util::stream::UrlSource * url_source_;
            util::stream::UrlSink * url_sink_;
            bool opened_;
            boost::mutex mutex_;
        };

    } // namespace download
} // namespace just

#endif // _JUST_DOWNLOAD_DISPATCH_DOWNLOADHER_H_
