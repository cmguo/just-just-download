// DownloadBase.h

#ifndef _PPBOX_DOWNLOAD_DOWNLOAD_BASE_H_
#define _PPBOX_DOWNLOAD_DOWNLOAD_BASE_H_

#include <framework/string/Url.h>

namespace ppbox {

    namespace download
    {

        typedef boost::function<void (
            boost::system::error_code const &)
        >  response_type;

    } // namespace download
} // namespace ppbox

#endif // _PPBOX_DOWNLOAD_DOWNLOAD_BASE_H_
