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
                already_open = 1, 
                not_open, 
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
                        if (value == error::already_open)
                            return "download: has already opened";
                        if (value == error::not_open)
                            return "download: has not opened";
                        return "download: unknown error";
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
