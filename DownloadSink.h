#ifndef _PPBOX_DOWNLOAD_RTPSINK_H_
#define _PPBOX_DOWNLOAD_RTPSINK_H_

#include <ppbox/mux/tool/Sink.h>

#include <fstream>

namespace ppbox
{
    namespace download
    {
        class UpReport;

        class DownloadSink 
            : public ppbox::mux::Sink
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
            virtual boost::system::error_code write(
                boost::posix_time::ptime const & time_send,
                ppbox::demux::Sample&);

        private:
            std::fstream file_;
            boost::uint32_t time_;  //查打时间值
        };

    } // namespace download
} // namespace ppbox

#endif 
