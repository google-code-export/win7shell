#ifndef versionchecker_h__
#define versionchecker_h__

#include <string>
#include <list>

#define MAXDATASIZE 1000

class VersionChecker
{
public:
	VersionChecker();
	~VersionChecker();

	std::wstring IsNewVersion(std::wstring curvers);
    int GetMessages(std::list<std::pair<std::wstring, std::wstring>>& message_list, int &last_message);

private:
	bool inited;
};

#endif // versionchecker_h__
