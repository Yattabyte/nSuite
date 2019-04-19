#include "Common.h"
#include "TaskLogger.h"
#include <map>

// Command inclusions
#include "Commands/Command.h"
#include "Commands/InstallerCommand.h"
#include "Commands/PackagerCommand.h"
#include "Commands/DiffCommand.h"
#include "Commands/PatchCommand.h"
#include "Commands/PackCommand.h"
#include "Commands/UnpackCommand.h"


/** Entry point. */
int main(int argc, char *argv[])
{
	// Load all commands into a command map
	const auto start = std::chrono::system_clock::now();
	struct compare_string { bool operator()(const char * a, const char * b) const { return strcmp(a, b) < 0; } };
	const std::map<const char *, Command*, compare_string> commandMap{ 
		{	"-installer"	,	new InstallerCommand()	},
		{	"-packager"		,	new PackagerCommand()	},
		{	"-pack"			,	new PackCommand()		},
		{	"-unpack"		,	new UnpackCommand()		},
		{	"-diff"			,	new DiffCommand()		},
		{	"-patch"		,	new PatchCommand()		}
	};
	TaskLogger::AddCallback_TextAdded([&](const std::string & message) {
		std::cout << message;
	});

	// Check for valid arguments
	if (argc <= 1 || commandMap.find(argv[1]) == commandMap.end())
		exit_program(
			"                      ~\r\n"
			"    nSuite Help:     /\r\n"
			"  ~-----------------~\r\n"
			" /\r\n"
			"~\r\n\r\n"
			" Operations Supported:\r\n"
			" -installer	(Packages a directory into a GUI Installer (for Windows))\r\n"
			" -packager		(Packages a directory into a portable package executable (like a mini installer))\r\n"
			" -pack			(Packages a directory into an .npack file)\r\n"
			" -unpack		(Unpackages into a directory from an .npack file)\r\n"
			" -diff			(Diff. 2 directories into an .ndiff file)\r\n"
			" -patch		(Patches a directory from an .ndiff file)\r\n"
			"\r\n\r\n"
		);
	
	// Command exists in command map, execute it
	commandMap.at(argv[1])->execute(argc, argv);

	// Output results and finish
	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> elapsed_seconds = end - start;
	TaskLogger::PushText("Total duration: " + std::to_string(elapsed_seconds.count()) + " seconds\r\n\r\n");
	system("pause");
	exit(EXIT_SUCCESS);
}