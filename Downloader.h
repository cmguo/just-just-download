//Downloader.h

#ifndef _PPBOX_DOWNLOAD_DOWNLOADER_H_
#define _PPBOX_DOWNLOAD_DOWNLOADER_H_

#include <framework/timer/TickCounter.h>

namespace ppbox {

    namespace download
    {

        struct DownloadStatistic
        {
            DownloadStatistic()
            {
                clear();
            }
            void clear()
            {
                total_size = 0;
                finish_size = 0;
                speed = 0;
            }
			boost::uint64_t total_size;  
            boost::uint64_t finish_size;
            boost::uint32_t speed; // B/s
        };

        class Downloader
        {

        public:
            typedef boost::function<void (
                boost::system::error_code const &)
            >  response_type;

            Downloader();

            virtual ~Downloader();

            virtual boost::system::error_code open(
                std::string const & playlink,
                std::string const & format,
                std::string const & filename,
                response_type const & resp) = 0;

            virtual boost::system::error_code close(
                boost::system::error_code & ec) = 0;

            virtual boost::system::error_code get_download_statictis(
                DownloadStatistic & download_statistic,
                boost::system::error_code & ec) = 0;
       
		protected:
			boost::uint32_t calc_speed(
					boost::uint64_t new_size);

		private:
            boost::uint64_t last_size_;
			framework::timer::TickCounter tick_count_;

        };

    }//download

}//ppbox


#endif
