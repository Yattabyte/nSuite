# Installer
The raw version of this application is useless on its own, and is intended to be fullfilled by nSuite using the `-Installer` command.
The following is how nSuite uses this application:
  - writes out a copy of this application to disk
  - packages a directory, embedding the package resource into **this application's** IDR_ARCHIVE resource
  - tries to find an Installer manifest, embedding it into **this application's** IDR_MANIFEST resource

This program is a fully-fledged Installation application, made to run on Windows. It generates an unInstaller, and links it up in the user's registry.

It uses the Windows GDI library for rendering.

This application has 3 (three) resources embedded within it:
  - IDI_ICON1		the application icon
  - IDR_ARCHIVE		the package to be Installed
  - IDR_MANIFEST	the Installer manifest (attributes, strings, iyattaructions)
  
This Installer has several screens it displays to the user. It starts off with a welcome screen, and requires that the user accept a EULA in order to proceed.
If at any point an error occurs, the program enters a failure state and dumps its entire operation log to disk (next to the program, error_log.txt).

The Installer manifest has optional strings the developer can implement to customize the Installation process. *Quotes are required*
  - name "string"
  - version "string"
  - description "string"
  - eula "string"
  - shortcut "\\relative path within Installation directory"
  - icon "\\relative path within Installation directory"
  
Any and all of these manifest values can be omitted.