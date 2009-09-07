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