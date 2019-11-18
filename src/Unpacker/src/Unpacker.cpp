#include "Log.h"
#include "Directory.h"
#include "Resource.h"
#include <chrono>
#include <iostream>


/** Entry point. */
int main()
{
	// Tap-in to the log, have it redirect to the console
	auto index = NST::Log::AddObserver([&](const std::string& message) {
		std::cout << message;
		});

	NST::Log::PushText(
		"                       ~\r\n"
		"       Un-packager     /\r\n"
		"  ~------------------~\r\n"
		" /\r\n"
		"~\r\n\r\n"
	);

	// Acquire archive resource
	const auto start = std::chrono::system_clock::now();
	const auto dstDirectory = NST::Directory::SanitizePath(NST::Directory::GetRunningDirectory());
	NST::Resource archive(IDR_ARCHIVE, "ARCHIVE");
	if (!archive.exists())
		NST::Log::PushText("Cannot access archive resource (may be absent, corrupt, or have different identifiers), aborting...\r\n");
	else {
		// Parse the header
		std::string packageName("");
		// Report where we're unpacking to
		NST::Log::PushText(
			"Unpacking to the following directory:\r\n"
			"\t> " + dstDirectory +
			"\r\n"
		);

		// Un-package using the resource file
		NST::Directory directory(NST::Buffer(archive.getPtr(), archive.getSize(), false), dstDirectory);
		if (!directory.apply_folder())
			NST::Log::PushText("Cannot decompress embedded package resource, aborting...\r\n");
		else {
			// Success, report results
			const auto end = std::chrono::system_clock::now();
			const std::chrono::duration<double> elapsed_seconds = end - start;
			NST::Log::PushText(
				"Files processed: " + std::to_string(directory.fileCount()) + "\r\n" +
				"Bytes processed: " + std::to_string(directory.byteCount()) + "\r\n"
				"Total duration: " + std::to_string(elapsed_seconds.count()) + " seconds\r\n\r\n"
			);

			// Pause and exit
			NST::Log::RemoveObserver(index);
			system("pause");
			return EXIT_SUCCESS;
		}
	}
	// Pause and exit
	NST::Log::RemoveObserver(index);
	system("pause");
	return EXIT_FAILURE;
}