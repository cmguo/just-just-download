#ifndef      _PPBOX_DOWNLOAD_STATISTIC_
#define      _PPBOX_DOWNLOAD_STATISTIC_

//#include <boost/date_time/posix_time/posix_time_types.hpp>
//#include <boost/date_time/time_duration.hpp>

#include <framework/timer/ClockTime.h>

//using namespace boost::date_time;
using namespace framework::timer;

namespace ppbox
{
    namespace download
    {
        //typedef boost::posix_time::ptime time_type;
        typedef boost::uint64_t size_type;
        struct DownloadStatistic
        {
            Time begin_time;
            size_type total_size;
            size_type finish_size;
            boost::uint32_t speed; // B/s
        };

        class PptvDownloadStatistic
        {
        public:
            PptvDownloadStatistic()
            {
            }

            ~PptvDownloadStatistic()
            {
            }

            void begin_statictis(void)
            {
                stat_.begin_time  = Time::now();
                stat_.finish_size = 0;
                stat_.total_size  = 0;
                stat_.speed       = 0;
            }

            void end_statictis(void)
            {
                stat_.speed       = 0;
            }

            void reset(void)
            {
                stat_.begin_time = Time::now();
            }

            void set_total_size(size_type size)
            {
                stat_.total_size = size;
            }

            void set_finish_size(size_type size)
            {
                stat_.finish_size = size;
            }

            void add_finish_size(size_type size)
            {
                //¼ÆËãËÙ¶È
                boost::uint32_t expend_time = (Time::now() - stat_.begin_time).total_seconds();
                stat_.finish_size += size;
                if (expend_time != 0) {
                    stat_.speed = ((size * 1000) / expend_time) * 8;
                    if (stat_.speed > 0) {
                        stat_.speed = stat_.speed/1024;
                    }
                }
            }

            DownloadStatistic get_download_stat(void)
            {
                return stat_;
            }

        private:
            DownloadStatistic stat_;
        };
    }
}


#endif /* _PPBOX_DOWNLOAD_STATISTIC_ */
