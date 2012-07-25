#ifndef _PPBOX_DOWNLOAD_COMMONTYPE_H_
#define _PPBOX_DOWNLOAD_COMMONTYPE_H_

namespace ppbox
{
    namespace download
    {
        //»ñÈ¡ÏÂÔØ×´Ì¬
        struct FileDownloadStatistic
        {
            FileDownloadStatistic()
            {
                clear();
            }
            void clear()
            {
                total_size = 0;
                finish_size = 0;
                speed = 0;
            }
			boost::uint64_t total_size;  
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

