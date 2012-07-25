//Mp4Downloader.cpp

#include "ppbox/download/Common.h"
#include "ppbox/download/Mp4Downloader.h"
#include "ppbox/download/Manager.h"
#include "ppbox/download/FileParse.h"
#include "ppbox/vod/BigMp4.h"

#include <util/protocol/pptv/Url.h>

#include <framework/logger/Logger.h>
#include <framework/logger/LoggerStreamRecord.h>

using namespace framework::logger;

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("Mp4Downloader", 0);

namespace ppbox
{
    namespace download
    {
        Mp4Downloader::Mp4Downloader(
				boost::asio::io_service & io_svc)
#if !defined(PPBOX_DISABLE_VOD) || !defined(PPBOX_DISABLE_PEER)
            : bigmp4_(io_svc)
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
            LOG_S(Logger::kLevelEvent, "[add] play_link:"<<playlink
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

            std::string oldUrl(playlink);
#if !defined(PPBOX_DISABLE_VOD)
            bigmp4_.async_open(oldUrl
                //,ppbox::vod::BigMp4::FetchMode::big_head
                ,ppbox::vod::BigMp4::FetchMode::small_head
                ,file_
                ,boost::bind(&Mp4Downloader::async_open_callback, this, _1));
#elif !defined(PPBOX_DISABLE_PEER)
            bigmp4_.async_open(oldUrl
                ,ppbox::peer::BigMp4::FetchMode::small_head
                //,ppbox::peer::BigMp4::FetchMode::big_head
                ,file_
                ,boost::bind(&Mp4Downloader::async_open_callback,this, _1));
#else
            ec = error::not_support_download;
#endif
            return ec;
        }

        void Mp4Downloader::async_open_callback(
            boost::system::error_code const & ec)
        {
            if(ec)
            {
                LOG_S(Logger::kLevelEvent, "[async_open_callback] ec:"<<ec.message());
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
            LOG_S(Logger::kLevelEvent, "[async_body_callback] ec:"<<ec.message());
            file_.close();

            resp_(ec);//回调
        }

		boost::system::error_code Mp4Downloader::close(
            boost::system::error_code & ec)
        {
            LOG_S(Logger::kLevelEvent, "[del]");
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
            
			download_statistic.total_size = total_size;
			download_statistic.finish_size = valid_size;
			download_statistic.speed = calc_speed(valid_size);
#endif
            ec.clear();
			return ec;
 		}


    }//download
}//ppbox

