#include "ppbox/download/Common.h"
#include "ppbox/download/FileParse.h"
#include "ppbox/download/DownloadError.h"

#include <ppbox/avformat/flv/FlvTagType.h>
#include <ppbox/mux/ts/MpegTsType.h>
#include <ppbox/mux/ts/Mpeg2Ts.h>

#include <util/archive/ArchiveBuffer.h>

#include <boost/filesystem/operations.hpp>
#include <fstream>
#include <framework/system/BytesOrder.h>



FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("DownloadFileParse", 0)

namespace ppbox
{
    namespace download
    {
        FileParse::~FileParse()
        {
        }

        FileParse::FileParse()
        {

        }

        boost::system::error_code FileParse::get_seektime(
            std::string const & fomat
            ,std::string const & filename
            , boost::uint32_t& seek_time
            , boost::uint32_t& seek_size)
        {
            boost::system::error_code ec;

            if(!is_exist(filename+".tmp"))
            {
                ec = ppbox::download::error::no_temp_file;
            }
            else if(fomat == "ts")
            {
                return parse_ts(filename,seek_time,seek_size);
            }
            else if(fomat == "flv")
            {
                return parse_flv(filename,seek_time,seek_size);
            }
            else if(fomat == "mp4")
            {
                return parse_mp4(filename,seek_time,seek_size);
            }
            else
            {
                ec = ppbox::download::error::format_not_support;
            }

            return ec;
        }

//private

        bool FileParse::is_exist(std::string const & filename)
        {
            return boost::filesystem::exists(filename.c_str());
        }

        void FileParse::destory(std::string const & filename)
        {
            if(is_exist(filename))
            {
                boost::filesystem::remove(filename);
            }
        }

         void FileParse::rename(std::string const & filename)
         {
            destory(filename);
            std::string tempfile = filename+".tmp";
            boost::filesystem::rename(tempfile,filename);
         }

        boost::system::error_code FileParse::get_filesize(
            std::string const & filename
            , boost::uint64_t& size)
        {
            boost::system::error_code ec;

            if(!is_exist(filename))
            {
                ec = ppbox::download::error::no_temp_file;
            }
            else
            {
                std::fstream read_file;
                read_file.open(filename.c_str(),std::ios::in| std::ios::binary);
                read_file.seekg(0,std::ios::end);
                size = read_file.tellg();
                read_file.close();
            }
            return ec;
        }

        //解析mp4 body size
        boost::system::error_code FileParse::parse_mp4(
            std::string const & filename
            , boost::uint32_t& seek_time
            , boost::uint32_t& seek_size)
        {
            boost::system::error_code ec;
            std::string full_path = filename + ".tmp";

            seek_size = 0;
            boost::uint32_t size = 0;

            std::fstream read_file;
            read_file.open(full_path.c_str(),std::ios::in| std::ios::binary);
            read_file.seekg(0,std::ios::end);
            size = read_file.tellg();
            seek_time = size;
            read_file.seekg(0,std::ios::beg);
            if(size < 24)
            {
                return ec;
            }


            boost::uint32_t size1;
            read_file.read((char*)&size1,4);
            size1 = framework::system::BytesOrder::host_to_net_long(size1);

            if (size1+4 > size) 
            {
                return ec;
            }

            read_file.seekg(size1-4,std::ios::cur);
            
            boost::uint32_t size2;
            read_file.read((char*)&size2,4);
            size2 = framework::system::BytesOrder::host_to_net_long(size2);

            boost::uint32_t head_size = size1 + size2 + 8;
            if(head_size > size)
            {
                seek_size = 0;
            }
            else
            {
                seek_size = size - head_size;
            }
            return ec;
        }

        boost::system::error_code FileParse::parse_ts(
            std::string const & filename
            , boost::uint32_t& seek_time
            , boost::uint32_t& seek_size)
        {
            boost::system::error_code ec;
            std::string full_path = filename + ".tmp";
            //先算出TsIArchive

            std::basic_fstream<char> read_file;
            ppbox::mux::TsIArchive archive(read_file);
            read_file.open(full_path.c_str(),std::ios::in| std::ios::binary);
            archive.seekg(0,std::ios::end);

            boost::uint32_t total_size = read_file.tellg();
            boost::uint32_t tsCount = total_size / AP4_MPEG2TS_PACKET_SIZE;

            ppbox::mux::TransportPacket tsHeader;
            ppbox::mux::PESPacket pesHeader;
            ppbox::mux::AdaptationField afHeader;
            while ( tsCount-- > 0)
            {
                archive.seekg(tsCount*AP4_MPEG2TS_PACKET_SIZE,std::ios::beg);
                archive>>tsHeader;

                if(!archive)
                {
                    std::cout<<"Seek tagesize failed"<<std::endl;
                    ec = ppbox::download::error::parse_temp_file_error;
                    break;
                }

                //if (1 == tsHeader.payload_uint_start_indicator)
                if (1 == tsHeader.payload_uint_start_indicator)
                {
                    std::cout<<"Has Find PES"<<std::endl;

                    if (2 == tsHeader.adaptat_field_control
                        || 3 == tsHeader.adaptat_field_control)
                    {
                        archive>>afHeader;
                    }

                    archive>>pesHeader;

                    boost::uint64_t value(0);
                    if (2 == pesHeader.PTS_DTS_flags)
                    {//PTS
                        boost::uint64_t temp = pesHeader.Pts32_30;
                        temp = temp<<30;
                        value += temp;

                        temp = pesHeader.Pts29_15;
                        temp = temp<<15;
                        value += temp;

                        value += pesHeader.Pts14_0;
                    }
                    else if (3 == pesHeader.PTS_DTS_flags)
                    {
                        boost::uint64_t temp = pesHeader.Dts32_30;
                        temp = temp<<30;
                        value += temp;

                        temp = pesHeader.Dts29_15;
                        temp = temp<<15;
                        value += temp;

                        value += pesHeader.Dts14_0;
                    }
                    else
                    {
                        assert(0);
                        break;
                    }

                    seek_time = value/90;
                    seek_size = tsCount*AP4_MPEG2TS_PACKET_SIZE;
                    break;
                }

            }
            
            return ec;
        }

        boost::system::error_code FileParse::parse_flv(
            std::string const & filename //std::basic
            , boost::uint32_t& seek_time
            , boost::uint32_t& seek_size)
        {
            try
            {
                boost::system::error_code ec;
                std::string full_path = filename + ".tmp";

                std::basic_fstream<boost::uint8_t> read_file;
                ppbox::avformat::FLVArchive archive(read_file);
                read_file.open(full_path.c_str(),std::ios::in| std::ios::binary);
                archive.seekg(0,std::ios::end);

                boost::uint32_t total_size = read_file.tellg();
                ppbox::avformat::FlvTagHeader tagheader;

                framework::system::UInt24 data_size;

                long  leave_sise = total_size-3;

                //定义FLV头的最小值 
#define FLV_HEADERS_SIZE 13

                boost::uint32_t tagsize = 0;

                while((leave_sise -=1) > FLV_HEADERS_SIZE)
                {
                    //移动文件指针到 leave_sise位置
                    archive.seekg(leave_sise,std::ios::beg);
                    archive >> tagsize;

                    if(!archive)
                    {
                        std::cout<<"Seek tagesize failed"<<std::endl;
                        ec = ppbox::download::error::parse_temp_file_error;
                        break;
                    }

                    if ((0 == tagsize) || (tagsize + FLV_HEADERS_SIZE > (leave_sise)))
                    {
                        continue;
                    }

                    archive.seekg(leave_sise-tagsize,std::ios::beg);
                    archive >> tagheader;

                    if(!archive)
                    {
                        std::cout<<"Seek tagheader failed"<<std::endl;
                        ec = ppbox::download::error::parse_temp_file_error;
                        break;
                    }

                    if (tagsize == tagheader.DataSize+11)
                    {
                        seek_time = tagheader.Timestamp;
                        seek_size = leave_sise-tagsize;
                        break;
                    }
                }
                return ec;

            }
            catch(...)
            {
                 return boost::system::error_code();
            }
        }
    } // namespace download
} // namespace ppbox
