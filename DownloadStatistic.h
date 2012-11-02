// DownloadStatistic.h

#ifndef _PPBOX_DOWNLOAD_DOWNLOAD_STATISTIC_H_
#define _PPBOX_DOWNLOAD_DOWNLOAD_STATISTIC_H_

#include "ppbox/download/DownloadBase.h"

namespace ppbox {

    namespace download
    {

        struct DownloadStatistic
        {
            DownloadStatistic()
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

    } // namespace download
} // namespace ppbox

#endif // _PPBOX_DOWNLOAD_DOWNLOAD_STATISTIC_H_
