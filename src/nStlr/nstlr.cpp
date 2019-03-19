#include "Archiver.h"
#include "Common.h"

// Command inclusions
#include <map>
#include "Commands/Command.h"
#include "Commands/DiffDirectory.h"
#include "Commands/PatchDirectory.h"


/** Entry point. */
int main(int argc, char *argv[])
{
	// Load all commands into a command map
	const auto start = std::chrono::system_clock::now();
	struct compare_string { bool operator()(const char * a, const char * b) const { return strcmp(a, b) < 0; } };
	const std::map<const char *, Command*, compare_string> commandMap{ {"-dd", new DiffDirectory()}, {"-pd", new PatchDirectory()} };

	// Check for valid arguments
	if (argc <= 1 || commandMap.find(argv[1]) == commandMap.end())
		exit_program("\n"
			"        Help:       /\n"
			" ~-----------------~\n"
			"/\n"
			" Operations Supported:\n"
			" -dd (To diff an entire directory)\n"
			" -pd (To patch an entire directory)\n"
			"\n\n"
		);
	
	// Command exists in command map, execute it
	commandMap.at(argv[1])->execute(argc, argv);
	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> elapsed_seconds = end - start;
	std::cout << "Total duration: " << elapsed_seconds.count() << " seconds\n\n";
	system("pause");
	exit(EXIT_SUCCESS);
}