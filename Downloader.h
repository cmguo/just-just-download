//Downloader.h

#ifndef _PPBOX_DOWNLOAD_DOWNLOADER_H_
#define _PPBOX_DOWNLOAD_DOWNLOADER_H_

#include "ppbox/download/DownloadBase.h"
#include "ppbox/download/DownloadStatistic.h"

#include <framework/timer/TickCounter.h>

namespace ppbox {

    namespace download
    {

        class Downloader
        {
        public:
            Downloader(
                boost::asio::io_service & io_svc);

            virtual ~Downloader();

        public:
            virtual void open(
                framework::string::Url const & url,
                response_type const & resp);

            virtual bool cancel(
                boost::system::error_code & ec) = 0;

            virtual bool close(
                boost::system::error_code & ec) = 0;

            virtual bool get_statictis(
                DownloadStatistic & stat,
                boost::system::error_code & ec) = 0;

        public:
            boost::asio::io_service & io_svc()
            {
                return io_svc_;
            }

        protected:
            void response(
                boost::system::error_code const & ec);

            boost::uint32_t calc_speed(
                boost::uint64_t new_size);

        private:
            boost::asio::io_service & io_svc_;
            response_type resp_;
            boost::uint64_t last_size_;
            framework::timer::TickCounter tick_count_;
        };

    } // namespace download
} // namespace ppbox

#endif
