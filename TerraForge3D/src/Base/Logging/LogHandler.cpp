#include "Base/Logging/LogHandler.h"


int LoggingOutputStreambuf::overflow( int ch )
{
	//myLogFile.sputc( ch );  //  ignores errors...
	myLogFile << (char)( ch );  //  ignores errors...
	return myDest->sputc( ch );
}

LoggingOutputStreambuf::LoggingOutputStreambuf(
    std::streambuf *dest,
    std::string const &filename )
	: myDest( dest )
	, myOwner( nullptr )
{
	myLogFile.open(filename);
	myLogFile  <<"Logging Started\n";

	if ( !myLogFile.is_open() )
	{
		//  Some error handling...
	}
}

LoggingOutputStreambuf::LoggingOutputStreambuf(
    std::ostream &dest,
    std::string const &logfileName )
	: LoggingOutputStreambuf( dest.rdbuf(), logfileName )
{
	dest.rdbuf( this );
	dest << "Starting Logger " << logfileName << "\n";
	myOwner = &dest;
}

LoggingOutputStreambuf::~LoggingOutputStreambuf()
{
	if ( myOwner != nullptr )
	{
		myOwner->rdbuf( myDest );
	}

	myLogFile.close();
}
