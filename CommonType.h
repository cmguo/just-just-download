#ifndef _PPBOX_DOWNLOAD_COMMONTYPE_H_
#define _PPBOX_DOWNLOAD_COMMONTYPE_H_

namespace ppbox
{
    namespace download
    {
        //获取下载状态
        struct FileDownloadStatistic
        {
            FileDownloadStatistic()
            {
                clear();
            }
            void clear()
            {
                finish_percent = 0.0;
                finish_size = 0;
                speed = 0;
            }
            float finish_percent;  //已下载百分比
            boost::uint64_t finish_size;
            boost::uint32_t speed; // B/s
        };

        struct MangerHandle
        {
            MangerHandle()
            {
                content = NULL;
            }
            MangerHandle(char const * format):type(format)
            {
                content = NULL;
            }
            void* content;
            std::string type;
        };
    }
}
#endif

