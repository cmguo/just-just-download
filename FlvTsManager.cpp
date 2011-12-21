// FlvTsManager.cpp
#include "ppbox/download/Common.h"
#include "ppbox/download/FlvTsManager.h"
#include "ppbox/download/DownloadDispatcher.h"


#include <utility>

using namespace boost::system;

namespace ppbox
{
    namespace download
    {
        FlvTsManager::FlvTsManager(util::daemon::Daemon & daemon)
			: ppbox::common::CommonModuleBase<FlvTsManager>(daemon, "FlvTsManager")
        {
        }

        FlvTsManager::~FlvTsManager()
        {
        }

        error_code FlvTsManager::startup()
        {
            return error_code();
        }

        void FlvTsManager::shutdown()
        {

        }

        error_code FlvTsManager::add(char const * playlink,
                                char const * format,
                                char const * dest,
                                char const * filename,
                                void* & download_hander,
                                error_code & ec)
        {

            //判断是新打开，还是断点续传
             DownloadDispatcher* dispatch  = get_freeDispatch();
            dispatch->open(playlink,format,dest,filename);
            download_hander = (void*)dispatch;
            return ec;
        }

        error_code FlvTsManager::del(void* const download_hander,
                                error_code & ec)
        {
            DownloadDispatcher* dispatch = (DownloadDispatcher*)download_hander;
            ec = dispatch->close();
            return ec;
        }

        error_code FlvTsManager::get_download_statictis(void* const download_hander,
                                                   FileDownloadStatistic & download_statistic,
                                                   error_code & ec)
        {
            DownloadDispatcher* dispatch = (DownloadDispatcher*)download_hander;
            dispatch->get_download_statictis(download_statistic,ec);
            return ec;
        }

        int DownloadCompare(  DownloadDispatcher* info)
        {
            return (!info->is_free());
        }

        boost::uint32_t FlvTsManager::get_downloader_count(void) const
        {
            boost::uint32_t iCount = std::count_if(dispatcher_.begin(),dispatcher_.end(),DownloadCompare);
            return iCount;
        }

        error_code FlvTsManager::get_last_error(void* const download_hander) const
        {
            error_code ec;
            return ec;

        }

         DownloadDispatcher* FlvTsManager::get_freeDispatch()
         {
             DownloadDispatcher* dispatch = NULL;

             std::vector<DownloadDispatcher*>::iterator iter = dispatcher_.begin();
             for(; iter != dispatcher_.end(); ++iter)
             {
                 if( (*iter)->is_free())
                 {
                    dispatch = (*iter);
                    break;
                 }
             }
             if(NULL == dispatch)
             {
                dispatch = new DownloadDispatcher(get_daemon());
                dispatch->start();
                dispatcher_.push_back(dispatch);
             }
             
             return dispatch;
         }
    }
}
