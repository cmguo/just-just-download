//Mp4Downloader.cpp

#include "ppbox/download/Common.h"
#include "ppbox/download/Mp4Downloader.h"
#include "ppbox/download/Manager.h"
#include "ppbox/download/FileParse.h"

#include <util/protocol/pptv/Url.h>

#include <framework/system/ErrorCode.h>
#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

using namespace framework::logger;

#ifndef PPBOX_DISABLE_VOD 
using namespace ppbox::vod;
#endif

#ifndef PPBOX_DISABLE_PEER
using namespace ppbox::peer;
#endif

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.download.Mp4Downloader", Debug);

namespace ppbox
{
    namespace download
    {
        Mp4Downloader::Mp4Downloader(
            boost::asio::io_service & io_svc)
#if !defined(PPBOX_DISABLE_VOD) || !defined(PPBOX_DISABLE_PEER)
            : io_srv_(io_svc)
            , bigmp4_(io_svc)
            , head_size_(0)
#else
            : head_size_(0)
#endif
        {
        }

        Mp4Downloader::~Mp4Downloader()
        {
        }

        boost::system::error_code Mp4Downloader::open(
            std::string  const & playlink, 
            std::string  const & format, 
            std::string  const & filename, 
            response_type const& resp)
        {
            LOG_INFO("[add] play_link:"<<playlink
                <<" format:"<<format
                <<" filename:"<<filename);

            boost::system::error_code ec;

            resp_ = resp;
            filename_ = filename;
            std::string tmpfile= filename_;

            FileParse pasr;
            if(!pasr.is_exist(tmpfile))
            {
                std::ofstream stream(tmpfile.c_str());
                stream.close();
            }

            file_.open(tmpfile.c_str(),std::ios::in|std::ios::out|std::ios::binary);
            if (file_.is_open()) 
            {
                std::string oldUrl(playlink);
                bool bighead = (std::string::npos == oldUrl.find("bighead=true"))?false:true;

#if !defined(PPBOX_DISABLE_VOD)
                if (bighead)
                {
                    bigmp4_.async_open(oldUrl
                        ,ppbox::vod::BigMp4::FetchMode::big_head
                        //,ppbox::vod::BigMp4::FetchMode::big_head
                        ,file_
                        ,boost::bind(&Mp4Downloader::async_open_callback, this, _1));
                }
                else
                {
                    bigmp4_.async_open(oldUrl
                        ,ppbox::vod::BigMp4::FetchMode::small_head
                        ,file_
                        ,boost::bind(&Mp4Downloader::async_open_callback, this, _1));
                }
#elif !defined(PPBOX_DISABLE_PEER)
                if (bighead)
                {
                    bigmp4_.async_open(oldUrl
                        ,ppbox::peer::BigMp4::FetchMode::big_head
                        ,file_
                        ,boost::bind(&Mp4Downloader::async_open_callback,this, _1));
                }
                else
                {
                    bigmp4_.async_open(oldUrl
                        ,ppbox::peer::BigMp4::FetchMode::small_head
                        ,file_
                        ,boost::bind(&Mp4Downloader::async_open_callback,this, _1));
                }
#else
                ec = error::not_support_download;
                io_srv_.post(boost::bind(&Mp4Downloader::respone, this, ec));
#endif
            } else 
            {
                ec = framework::system::last_system_error();
                if (!ec)
                    ec = error::save_file_error;
                io_srv_.post(boost::bind(&Mp4Downloader::respone, this, ec));
            }
            return ec;
        }

        void Mp4Downloader::async_open_callback(
            boost::system::error_code const & ec)
        {
            if(ec)
            {
                LOG_INFO("[async_open_callback] ec:"<<ec.message());
                resp_(ec);
            }
            else
            {
#if !defined(PPBOX_DISABLE_VOD) || !defined(PPBOX_DISABLE_PEER)
                // 对size 判断是下体还是下header
                boost::uint32_t downSize = 0 ;
                bigmp4_.get_valid_size(downSize);
                file_.seekp(downSize,std::ios::beg);

                bigmp4_.async_tranfer(
                    downSize
                    ,file_
                    ,boost::bind(&Mp4Downloader::async_body_callback, this, _1));
#endif
            }
        }

        void Mp4Downloader::async_body_callback(
            boost::system::error_code const & ec)
        {
            LOG_INFO("[async_body_callback] ec:"<<ec.message());
            file_.close();

            resp_(ec);//回调
        }

        boost::system::error_code Mp4Downloader::close(
            boost::system::error_code & ec)
        {
            LOG_INFO("[del]");
#if !defined(PPBOX_DISABLE_VOD) || !defined(PPBOX_DISABLE_PEER)
            bigmp4_.cancel();
#endif
            return ec;
        }

        boost::system::error_code Mp4Downloader::get_download_statictis(
            DownloadStatistic & download_statistic, 
            boost::system::error_code & ec)
        {
#if !defined(PPBOX_DISABLE_VOD) || !defined(PPBOX_DISABLE_PEER)
            boost::uint32_t total_size = 0;
            boost::uint32_t valid_size = 0;
            bigmp4_.get_total_size(total_size);
            bigmp4_.get_valid_size(valid_size);
            BigMp4Info info;
            bigmp4_.get_info_statictis(info);

            download_statistic.total_size = total_size;
            download_statistic.finish_size = valid_size;
            download_statistic.speed = info.speed;
#endif
            ec.clear();
            return ec;
         }

        void Mp4Downloader::respone(boost::system::error_code const & ec)
        {
            resp_(ec);
        }


    }//download
}//ppbox

