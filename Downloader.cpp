//Downloader.cpp

#include "ppbox/download/Common.h"
#include "ppbox/download/Downloader.h"

#include <boost/bind.hpp>

namespace ppbox
{
    namespace download
    {

        Downloader::Downloader(
            boost::asio::io_service & io_svc)
            : io_svc_(io_svc)
            , last_size_(0)
        {
        }

        Downloader::~Downloader()
        {
        }

        void Downloader::open(
            framework::string::Url const & url,
            response_type const & resp)
        {
            resp_ = resp;
        }

        void Downloader::response(
            boost::system::error_code const & ec)
        {
            response_type resp;
            resp.swap(resp_);
            io_svc_.post(boost::bind(resp, ec));
        }

        boost::uint32_t Downloader::calc_speed(
            boost::uint64_t new_size)
        {
            boost::uint32_t elapse = tick_count_.elapsed();
            if (elapse == 0)
                return 0;
            boost::uint32_t speed = (boost::uint32_t)((new_size - last_size_) * 1000 / elapse);
            last_size_ = new_size;
            tick_count_.reset();
            return speed; 
        }

    }//download
}//ppbox

