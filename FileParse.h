#ifndef _PPBOX_DOWNLOAD_FILE_PARSE_H_
#define _PPBOX_DOWNLOAD_FILE_PARSE_H_


namespace ppbox
{
    namespace download
    {

        class FileParse 
        {
        public:
            FileParse(); 
            virtual ~FileParse();

            boost::system::error_code get_seektime(
                std::string const & fomat
                ,std::string const & filename
                , boost::uint32_t& seek_time
                , boost::uint32_t& seek_size);

            boost::system::error_code get_filesize(
                std::string const & filename
                , boost::uint64_t& size);

            bool is_exist(std::string const & filename);

            void destory(std::string const & filename);

            void rename(std::string const & filename);
        private:
             boost::system::error_code parse_mp4(
                 std::string const & filename
                 , boost::uint32_t& seek_time
                 , boost::uint32_t& seek_size);

            boost::system::error_code parse_ts(
                std::string const & filename
                , boost::uint32_t& seek_time
                , boost::uint32_t& seek_size);

            boost::system::error_code parse_flv(
                std::string const & filename
                , boost::uint32_t& seek_time
                , boost::uint32_t& seek_size);
        };

    } // namespace download
} // namespace ppbox

#endif 