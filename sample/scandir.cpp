#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#include <string>
#include <vector>
#include <iostream>
#include "../call_thunk.h"

using namespace std;

class dir
{
public:
	dir(const char* name) : _name(name) { }
	std::vector<std::string> find_file(const char* pattern) const
	{
		struct dirent **namelist;
		int n;
		filter f(pattern);
		std::vector<std::string> result;
		call_thunk::unsafe_thunk thunk(3, 0);
		thunk.bind(f, &filter::execute);
		n = scandir(".", &namelist, thunk, alphasort);
		if (n > 0)
		{
			while(n--)
			{
				result.push_back(namelist[n]->d_name);
				free(namelist[n]);
			}
			free(namelist);
		}	
		return result;
	}
	
private:

	struct filter
	{
		filter(const char* pattern) : _pattern(pattern) { }
		bool execute(const struct dirent * d) const
		{
			return fnmatch(_pattern, d->d_name, 0)==0; 
		}

		const char* _pattern;
	};

	std::string _name;
};

int main()
{
	dir d(".");
	std::vector<std::string> result = d.find_file("*.cpp");
	for(size_t i=0; i!=result.size(); i++)
		cout<<result[i]<<endl;
}


