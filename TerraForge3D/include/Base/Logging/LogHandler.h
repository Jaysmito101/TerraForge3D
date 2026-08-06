#include <fstream>
#include <iostream>

namespace tf3d::base
{

    class LoggingOutputStreambuf : public std::streambuf
    {
        std::streambuf *myDest;
        std::ofstream myLogFile;
        std::ostream *myOwner;

    protected:
        int overflow(int ch);

    public:
        LoggingOutputStreambuf(
            std::streambuf *dest,
            std::string const &logfileName);

        LoggingOutputStreambuf(
            std::ostream &dest,
            std::string const &logfileName);

        ~LoggingOutputStreambuf();
    };

} // namespace tf3d::base
using tf3d::base::LoggingOutputStreambuf;