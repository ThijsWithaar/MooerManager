#include "MooerManager.h"

#include <iostream>

#include "QExceptionApplication.h"



int main(int argc, char* argv[])
{
	QExceptionApplication app(argc, argv, "Mooer Manager");
	/*
		// This determines the filename of the settings
		app.setApplicationName(PROJECT_NAME);
		app.setOrganizationName(PACKAGE_VENDOR);
		app.setOrganizationDomain("mooer.net");
		app.setApplicationVersion(PROJECT_VERSION);
	*/
	app.setApplicationName("Mooer Manager");
	app.setDesktopFileName("MooerManager"); // This should match the "mooerManager.desktop" stem.

	int rc = -2;
	try
	{
		MooerManager main(nullptr);
		main.show();
		rc = app.exec();
	}
	catch(std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}
	return rc;
}
