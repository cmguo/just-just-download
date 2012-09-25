//Mp4Downloader.h

#ifndef _PPBOX_DOWNLOAD_MP4DOWNLOAD_H_
#define _PPBOX_DOWNLOAD_MP4DOWNLOAD_H_

#include "ppbox/download/Downloader.h"
#include "ppbox/download/CommonType.h"
#include "ppbox/segment/BigMp4.h"

#include <fstream>

namespace ppbox 
{
    namespace download
    {
        class Mp4Downloader 
            : public Downloader
        {

        public:
            Mp4Downloader(boost::asio::io_service & io_svc);

            ~Mp4Downloader();

            boost::system::error_code open(
                    std::string const & playlink,
                    std::string const & format,
                    std::string const & filename,
                    response_type const & resp);

            boost::system::error_code close(
                    boost::system::error_code & ec);

            boost::system::error_code get_download_statictis(
                DownloadStatistic & download_statistic,
                boost::system::error_code & ec);

            private:
                void async_open_callback(
                    boost::system::error_code const & ec);

            void async_body_callback(
                    boost::system::error_code const & ec);

        private:
            void respone(boost::system::error_code const & ec);

        private:
            boost::asio::io_service & io_srv_;
            ppbox::segment::BigMp4 bigmp4_;
            std::string filename_;
            boost::uint32_t head_size_;
            std::fstream file_;
            response_type  resp_;
        };

    }//download

}//ppbox

#endif

