#ifndef versionchecker_h__
#define versionchecker_h__

#include <string>

#define MAXDATASIZE 1000

class VersionChecker
{
public:
	VersionChecker();
	~VersionChecker();

	std::wstring IsNewVersion(std::wstring curvers);

private:
	bool inited;
};

#endif // versionchecker_h__
