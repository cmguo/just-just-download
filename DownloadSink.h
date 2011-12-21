#ifndef _PPBOX_DOWNLOAD_RTPSINK_H_
#define _PPBOX_DOWNLOAD_RTPSINK_H_

#include <ppbox/mux/tool/Sink.h>
#include <string>
#include <fstream>

namespace ppbox
{
    namespace download
    {
        class UpReport;

        class DownloadSink : public ppbox::mux::Sink
        {
        public:
            //DownloadSink();  ��ʱ��
            DownloadSink(std::string filename,UpReport* upobj,boost::uint32_t iTime,boost::uint32_t iSeek); 
            virtual ~DownloadSink();

            boost::system::error_code on_finish( boost::system::error_code const &ec);
        private:
            virtual boost::system::error_code write(ppbox::demux::Sample&);

        private:
            std::fstream file_;
            UpReport* upobj_;
            std::string filename_; 
            boost::uint32_t time_;  //���ʱ��ֵ
        };

    } // namespace download
} // namespace ppbox

#endif 