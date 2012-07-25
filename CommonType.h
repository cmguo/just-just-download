// CommonType.h

#ifndef _PPBOX_DOWNLOAD_COMMONTYPE_H_
#define _PPBOX_DOWNLOAD_COMMONTYPE_H_

namespace ppbox
{
    namespace download
    {

		enum DownloadStatusEnum {
			working,
			canceling,
			stopped,
			deleted,
			unknow
		};

		class Downloader;

        struct DownloadInfo
        {
            DownloadInfo()
            {
                downloader = NULL;
				cur_status = unknow;
            }
            Downloader * downloader;
			DownloadStatusEnum cur_status;
			Downloader::response_type resp;  
        };

    }//download
}//ppbox
#endif

