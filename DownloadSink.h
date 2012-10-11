#ifndef _PPBOX_DOWNLOAD_RTPSINK_H_
#define _PPBOX_DOWNLOAD_RTPSINK_H_

#include <ppbox/data/Sink.h>

#include <fstream>

namespace ppbox
{
    namespace download
    {
        class UpReport;

        class DownloadSink 
            : public ppbox::data::Sink
        {
        public:
            DownloadSink(
                std::string filename, 
                boost::uint32_t iTime, 
                boost::uint32_t iSeek);

            virtual ~DownloadSink();

            boost::system::error_code on_finish(
                boost::system::error_code const &ec);

        private:
            virtual size_t write(
                boost::posix_time::ptime const & time_send,
                ppbox::demux::Sample&,
                boost::system::error_code&);

        private:
            std::fstream file_;
            boost::uint32_t time_;  //查打时间值
        };

    } // namespace download
} // namespace ppbox

#endif 
