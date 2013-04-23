// DispatchDownloader.h

#ifndef _PPBOX_DOWNLOAD_DISPATCH_DOWNLOADHER_H_
#define _PPBOX_DOWNLOAD_DISPATCH_DOWNLOADHER_H_

#include "ppbox/download/Downloader.h"

#include <ppbox/dispatch/DispatchBase.h>

namespace ppbox
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
            ppbox::dispatch::DispatcherBase * dispatcher_;
            framework::string::Url url_;
            //ppbox::data::UrlSource * url_source_;
            ppbox::data::UrlSink * url_sink_;
            bool opened_;
        };

    } // namespace download
} // namespace ppbox

#endif // _PPBOX_DOWNLOAD_DISPATCH_DOWNLOADHER_H_
