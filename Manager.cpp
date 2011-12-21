// Manager.cpp
#include "ppbox/download/Common.h"
#include "ppbox/download/Mp4Manager.h"
#include "ppbox/download/FlvTsManager.h"
#include "ppbox/download/Manager.h"

#include <utility>

#include <boost/bind.hpp>

using namespace boost::system;

namespace ppbox
{
    namespace download
    {
        Manager::Manager(util::daemon::Daemon & daemon)
#ifdef  PPBOX_DISABLE_CERTIFY
			: ppbox::common::CommonModuleBase<Manager>(daemon, "download")
#else
            : ppbox::certify::CertifyUserModuleBase<Manager>(daemon, "download")
#endif            
            , mp4_module_(util::daemon::use_module<ppbox::download::Mp4Manager>(daemon))
            , flvts_module_(util::daemon::use_module<ppbox::download::FlvTsManager>(daemon))
            , io_srv_(io_svc())
        {
        }

        Manager::~Manager()
        {
        }

        error_code Manager::startup()
        {
        #ifndef  PPBOX_DISABLE_CERTIFY
            start_certify();
		#endif
            return error_code();
        }

        void Manager::shutdown()
        {
         #ifndef  PPBOX_DISABLE_CERTIFY
            stop_certify();
		 #endif

            
        }

        // 进入认证成功状态
        void Manager::certify_startup()
        {
            std::cout << "certify_startup" << std::endl;
        }

        // 退出认证成功状态
        void Manager::certify_shutdown(
            boost::system::error_code const & ec)
        {
            std::cout << "certify_shutdown" << std::endl;
        }

        // 认证出错
        void Manager::certify_failed(
            boost::system::error_code const & ec)
        {
            Manager::certify_shutdown(ec);
        }

        error_code Manager::add(char const * playlink,
                                char const * format,
                                char const * dest,
                                char const * filename,
                                DownloadHander & download_hander,
                                error_code & ec)
        {
            MangerHandle* handle = new MangerHandle(format);
            if(handle->type == "mp4" )
            {
                mp4_module_.add(playlink,format,dest,filename,handle->content,ec);
            }
            else
            {
                flvts_module_.add(playlink,format,dest,filename,handle->content,ec);
            }
            download_hander = (DownloadHander)handle;
            return ec;
        }

        error_code Manager::del(DownloadHander const download_hander,
                                error_code & ec)
        {
            MangerHandle* handle = (MangerHandle*)download_hander;

            if(handle->type == "mp4" )
            {
                mp4_module_.del(handle->content,ec);
            }
            else
            {
                flvts_module_.del(handle->content,ec);
            }
            
            //清内存
            delete handle;

            return ec;
        }

        error_code Manager::get_download_statictis(DownloadHander const download_hander,
                                                   DownloadStatistic & download_statistic,
                                                   error_code & ec)
        {
            FileDownloadStatistic temp;
            MangerHandle* handle = (MangerHandle*)download_hander;

            if(handle->type == "mp4" )
            {
                mp4_module_.get_download_statictis(handle->content,temp,ec);
            }
            else
            {
                flvts_module_.get_download_statictis(handle->content,temp,ec);
            }
            
            download_statistic.finish_percent = temp.finish_percent;
            download_statistic.finish_size = temp.finish_size;
            download_statistic.speed = temp.speed;

            return ec;
        }

        boost::uint32_t Manager::get_downloader_count(void) const
        {

            return (mp4_module_.get_downloader_count()+flvts_module_.get_downloader_count());
        }

        error_code Manager::get_last_error(DownloadHander const download_hander) const
        {
            error_code ec;
            return ec;

        }

        void Manager::set_download_path(char const * path)
        {

        }
    }
}
