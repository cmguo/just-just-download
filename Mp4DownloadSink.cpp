#include "ppbox/download/Common.h"
#include "ppbox/download/Mp4DownloadSink.h"

#include <ppbox/demux/DemuxerBase.h>
#include <boost/filesystem/operations.hpp>
#include <ppbox/demux/DemuxerError.h>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("Mp4DownloadSink", 0)

namespace ppbox
{
    namespace download
    {
        Mp4DownloadSink::~Mp4DownloadSink()
        {
            if(file_.is_open())
                file_.close();
        }

        Mp4DownloadSink::Mp4DownloadSink(
            std::string const & filename
            ,std::string const & format
            ,boost::uint64_t iSeek)
            :filename_(filename)
            ,format_(format)
        {
            try
            {
                std::string temp = filename.c_str();
                temp += format_;
                if(iSeek)
                {
                    file_.open(temp.c_str(),std::ios::binary | std::ios::in | std::ios::out);
                    file_.seekp(iSeek,std::ios::beg);
                }
                else
                {
                    file_.open(temp.c_str(),std::ios::binary | std::ios::trunc | std::ios::out);
                }
                
            }
            catch(...)
            {
                std::cout<<"Open "<<filename<<" Error"<<std::endl;
            }
        }

        boost::system::error_code Mp4DownloadSink::on_finish( 
            boost::system::error_code const &ec)
        {
            try
            {
                if(!ec || ec == ppbox::demux::error::no_more_sample)
                {
                    if(file_.is_open())
                        file_.close();
                    std::string tempfile = filename_+ format_;
                    boost::filesystem::rename(tempfile,filename_);
                    return boost::system::error_code();
                }
            }
            catch(...)
            {
                return boost::system::error_code();

            }

            return boost::system::error_code();
        }

        //工作线程调用
        boost::system::error_code Mp4DownloadSink::write( const char* buf,boost::uint64_t size)
        {
            boost::system::error_code ec;
            try
            {
                file_.write(buf,size);
                return ec;
            }
            catch(...)
            {
                ec = error::save_file_error;
                return ec;
            }
        }
    } // namespace download
} // namespace ppbox
