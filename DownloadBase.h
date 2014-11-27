// DownloadBase.h

#ifndef _JUST_DOWNLOAD_DOWNLOAD_BASE_H_
#define _JUST_DOWNLOAD_DOWNLOAD_BASE_H_

#include <framework/string/Url.h>

namespace just {

    namespace download
    {

        typedef boost::function<void (
            boost::system::error_code const &)
        >  response_type;

    } // namespace download
} // namespace just

#endif // _JUST_DOWNLOAD_DOWNLOAD_BASE_H_
