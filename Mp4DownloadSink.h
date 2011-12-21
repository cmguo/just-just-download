#ifndef _PPBOX_MP4_DOWNLOAD_RTPSINK_H_
#define _PPBOX_MP4_DOWNLOAD_RTPSINK_H_

#include <ppbox/mux/tool/Sink.h>
#include <string>
#include <fstream>

namespace ppbox
{
    namespace download
    {

        class Mp4DownloadSink : public ppbox::mux::Sink
        {
        public:
            //DownloadSink();  ×ÜÊ±³¤
            Mp4DownloadSink(
                std::string const & filename
                ,std::string const & format
                ,boost::uint64_t iSeek); 
            virtual ~Mp4DownloadSink();

            boost::system::error_code on_finish( boost::system::error_code const &ec);
            virtual boost::system::error_code write( const char* buf,boost::uint64_t size);

        private:
            std::fstream file_;
            std::string filename_; 
            std::string format_; 
        };

    } // namespace download
} // namespace ppbox

#endif 