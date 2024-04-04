# Cmake Lib Creator

## Overview
This editor is designed to create and maintain libraries created using the [template library](https://github.com/KROIA/QT_cmake_library_template).

## Table of content
-   [Description](#description)
-   [Installation](#installation)
-   [Create a library Project](Documentation/CreateLibraryProject.md)
-   [Edit a existing library Project](Documentation/EditLibraryProject.md)
-   [Upgrade an existing library Project](Documentation/UpgradeLibraryProject.md)
-   [Build this application](Documentation/build.md)

## Description
Because I'm a lazy person, I like to have a lot of work done for me through automation.
For many programming projects I use self-made libraries which are all based on CMake. This has allowed me to build a flexible structure with which I can realize various projects. With a standardized template implemented in each library, I can quickly implement new projects with little effort.
However, because the template can also be developed further on an ongoing basis, I was looking for a way to apply adaptations to the template to all existing projects with little effort.
The result is this application. 
It helps me to automatically transfer the changes in the cmake files and c++ code from the template to my existing libraries.
You can also create a new library directly with the application.
The tool provides all the required files and changes all placeholders in the code to suit the project.

The application accesses my repository with the template in order to always be able to use the latest changes.

Not happy with my template?
No problem, you can specify from which repository the templates should be downloaded.

### Features
- Create a new cmake library project
- Upgrade existing libraries, created with the template library.

## Installation
[Download](https://github.com/KROIA/CmakeLibCreator/releases) the compiled binary or [compile the project manually](Documentation/build.md).
Unzip the folder to any destination. 

## Warranty
This tool is used to make automated code changes. It is still possible that the parser not perfect and therefore replaces code incorrectly under certain conditions. I therefore recommend saving the library to be edited/upgraded before using this tool so that any errors can be quickly undone. 
I assume no liability for any problems that may occur in such an incident.

Please contact me if you notice that something has been edited incorrectly so that I can incorporate this into future versions.
Thank you for your understanding

## Contact
alexkrieg@gmx.ch




