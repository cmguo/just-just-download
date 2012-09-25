//Downloader.cpp

#include "ppbox/download/Common.h"
#include "ppbox/download/Downloader.h"

namespace ppbox
{
    namespace download
    {

        Downloader::Downloader()
            : last_size_(0)
        {
        }

        Downloader::~Downloader()
        {
        }

        boost::uint32_t Downloader::calc_speed(
            boost::uint64_t new_size)
        {
            boost::uint32_t elapse = tick_count_.elapsed();
            if (elapse == 0)
                return 0;
            boost::uint32_t speed = (new_size - last_size_) * 1000 / elapse;
            last_size_ = new_size;
            tick_count_.reset();
            return speed; 
        }

    }//download
}//ppbox

