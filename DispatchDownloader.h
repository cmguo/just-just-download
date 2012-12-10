// DispatchDownloader.h

#ifndef _PPBOX_DOWNLOAD_DISPATCH_DOWNLOADHER_H_
#define _PPBOX_DOWNLOAD_DISPATCH_DOWNLOADHER_H_

#include "ppbox/download/Downloader.h"

#include <ppbox/dispatch/Sink.h>

namespace ppbox
{
    namespace data
    {
        class FileSink;
    }

    namespace dispatch
    {
        class DispatcherBase;
        class Sink;
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
            //call back
            void handle_open(
                boost::system::error_code const & ec);

            void handle_play(
                boost::system::error_code const & ec);

        private:
            ppbox::dispatch::DispatcherBase * dispatcher_;
            ppbox::data::FileSink * file_sink_;
            ppbox::dispatch::Sink * sink_;
            bool opened_;
        };

    } // namespace download
} // namespace ppbox

#endif // _PPBOX_DOWNLOAD_DISPATCH_DOWNLOADHER_H_
