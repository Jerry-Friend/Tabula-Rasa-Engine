#ifndef __IMPORTER_H__
#define __IMPORTER_H__

#include "trDefs.h"
#include <string>

class Importer
{
public:

	Importer() {}
	virtual ~Importer() {}

	virtual bool Import(const char* file_path, std::string& output_file) = 0;

};


#endif