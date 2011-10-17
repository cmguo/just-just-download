// Manager.cpp
#include "ppbox/download/Common.h"
#include "ppbox/download/Manager.h"

#include <utility>

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

            DownloadInsideHander *hander = new DownloadInsideHander;
            hander->dest = dest;
            hander->format = format;
            hander->playlink = playlink;
            hander->downloder.reset(new Downloader(io_srv_));
            if (storage_path_.empty()) {
                hander->downloder->set_download_path(dest);
            } else {
                hander->dest = storage_path_;
                hander->downloder->set_download_path(storage_path_.c_str());
            }
            std::string end_filename = filename;
            if (end_filename.empty()) {
                ec = error::parameter_error;
            } else {
                end_filename += ".";
                end_filename += format;
                hander->downloder->set_file_name(end_filename.c_str());
                if (find(download_hander)) {
                    ec = error::already_download;
                } else {
                    hander->downloder->start(playlink, ec);
                    if (!ec) {
                        downloader_list_.push_back(hander);
                        download_hander = hander;
                    }
                }
            }
            
            return ec;
        }

        error_code Manager::del(DownloadHander const download_hander,
                                error_code & ec)
        {
            bool res = false;
            for (DownloaderList::iterator iter = downloader_list_.begin();
                iter != downloader_list_.end();
                iter++) {
                    DownloadInsideHander *tmp_handle = *iter;
                    if (tmp_handle == (DownloadInsideHander *)download_hander) {
                            tmp_handle->downloder->stop();
                            tmp_handle->downloder.reset();
                            downloader_list_.erase(iter);
                            delete tmp_handle;
                            res = true;
                            break;
                    }
            }

            if (!res) {
                ec = error::not_found_downloader;
            }
            return ec;
        }

        error_code Manager::get_download_statictis(DownloadHander const download_hander,
                                                   DownloadStatistic & download_statistic,
                                                   error_code & ec)
        {
            bool res = false;
            for (DownloaderList::const_iterator iter = downloader_list_.begin();
                iter != downloader_list_.end();
                iter++) {
                    DownloadInsideHander *tmp_handle = *iter;
                    if (tmp_handle == (DownloadInsideHander *)download_hander) {
                            res = true;
                            tmp_handle->downloder->get_download_statictis(download_statistic, ec);
                            break;
                    }
            }

            if (!res) {
                ec = error::not_found_downloader;
            }
            return ec;
        }

        boost::uint32_t Manager::get_downloader_count(void) const
        {
            return (boost::uint32_t)downloader_list_.size();
        }

        error_code Manager::get_last_error(DownloadHander const download_hander) const
        {
            bool res = false;
            for (DownloaderList::const_iterator iter = downloader_list_.begin();
                iter != downloader_list_.end();
                iter++) {
                    DownloadInsideHander *tmp_handle = *iter;
                    if (tmp_handle == (DownloadInsideHander *)download_hander) {
                            return tmp_handle->downloder->get_last_error();
                    }
            }

            error_code ec;
            if (!res) {
                ec = error::not_found_downloader;
            }
            return ec;

        }

        void Manager::set_download_path(char const * path)
        {
            storage_path_ = path;
        }

        bool Manager::find(DownloadHander const download_hander) const
        {
            bool res = false;
            for (DownloaderList::const_iterator iter = downloader_list_.begin();
                 iter != downloader_list_.end();
                 iter++) {
                 DownloadInsideHander *tmp_handle = *iter;
                 if (tmp_handle == (DownloadInsideHander *)download_hander) {
                     res = true;
                     break;
                 }
            }
            return res;
        }

    }
}
