// DownloadError.h

#ifndef _PPBOX_DOWNLOAD_ERROR_H_
#define _PPBOX_DOWNLOAD_ERROR_H_

namespace ppbox
{
    namespace download
    {

        namespace error {

            enum errors
            {
                access_jump_error = 1, 
                access_drag_error,
                bad_jump_file_format,
                bad_drag_file_format,
                not_start_download_data,
                open_download_link_failed,
                download_network_error,
                download_data_overflow,
                download_canceled,
                invalid_mp4_head,
                invalid_mp4_truck,
                not_found_any_segment,
                not_found_downloader,
                join_mp4_segment_failed,
                download_finished_task,
                already_download,
                parameter_error,
                save_file_error,
                format_not_support,
                no_temp_file,
                parse_temp_file_error,
                not_support_download,
                other_error,
            };

            namespace detail {

                class download_category
                    : public boost::system::error_category
                {
                public:
                    const char* name() const
                    {
                        return "download";
                    }

                    std::string message(int value) const
                    {
                        switch (value) {
                            case access_jump_error:
                                return "access info from jump server error";
                            case access_drag_error:
                                return "access info from drag server error";
                            case bad_jump_file_format:
                                return "bad jump xml file format";
                            case bad_drag_file_format:
                                return "bad drag xml file format";
                            case not_start_download_data:
                                return "not start download data";
                            case open_download_link_failed:
                                return "open download link failed";
                            case download_network_error:
                                return "download network error";
                            case download_data_overflow:
                                return "download data overflow";
                            case download_canceled:
                                return "download is canceled";
                            case invalid_mp4_head:
                                return "invalid mp4 head";
                            case invalid_mp4_truck:
                                return "invalid mp4 truck";
                            case not_found_any_segment:
                                return "not found any segment";
                            case not_found_downloader:
                                return "not found downloader";
                            case join_mp4_segment_failed:
                                return "join mp4 segment failed";
                            case download_finished_task:
                                return "download finished task";
                            case already_download:
                                return "already download";
                            case parameter_error:
                                return "parameter error";
                            case save_file_error:
                                return "save video error";
                            case format_not_support:
                                return "format not support";
                            case no_temp_file:
                                return "no temp file";
                            case parse_temp_file_error:
                                return "parse temp file error";
                            case not_support_download:
                                return "no vod or no peer,don't support download";
                            default:
                                return "unknown error";
                        }
                    }
                };
            } // namespace detail

            inline const boost::system::error_category & get_category()
            {
                static detail::download_category instance;
                return instance;
            }

            static boost::system::error_category const & download_category = get_category();

            inline boost::system::error_code make_error_code(
                errors e)
            {
                return boost::system::error_code(
                    static_cast<int>(e), get_category());
            }

        } // namespace error

    } // namespace download
} // namespace ppbox

namespace boost
{
    namespace system
    {

        template<>
        struct is_error_code_enum<ppbox::download::error::errors>
        {
            BOOST_STATIC_CONSTANT(bool, value = true);
        };

#ifdef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
        using ppbox::download::error::make_error_code;
#endif

    }
}

#endif // _PPBOX_DOWNLOAD_ERROR_H_
