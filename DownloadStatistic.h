// DownloadStatistic.h

#ifndef _JUST_DOWNLOAD_DOWNLOAD_STATISTIC_H_
#define _JUST_DOWNLOAD_DOWNLOAD_STATISTIC_H_

#include "just/download/DownloadBase.h"

namespace just {

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
} // namespace just

#endif // _JUST_DOWNLOAD_DOWNLOAD_STATISTIC_H_
