# Build {#build}
This application was developed using Visual Studio 2022.
To minimize any problems I recommend you to also use Visual Studio.

## Requirements
- CMake 3.20 or higher
- c++14 or higher
- QT 5.15.x (May also work with versions 5.x.x, but this is not tested)
  Used QT modules:
      - Core
      - Widgets 
  
### Recommended
It is recommended to use Visual Studio, since this is the development enviroment used to create this project.
To edit QT-Widgets, the [Visual Studio plugin](https://doc.qt.io/qtvstools/index.html) is a great addon.


## Compilation {#compilation}
1. Open the Project as CMake Project.
2. Let CMake download the dependencies.
3. Select the CmakeLibCreator.exe from the start target and press build.

The binary will be created in the location: **./build**
The application automaticly gets deployed using **windeployqt.exe** to that location.

## Troubleshooting
### QT not found
The **QtLocator.cmake** file, located in **./cmake** will automaticly try to find the newest QT5 version installed.
If it can't find any installation, check the following:

- Is QT5 installed?
- QT installation path: **C:\Qt\5.x.x**


### Cmake errors
Some Cmake errors can be resolved by generating the Cmake cache from scratch.
To do so, go to Visual Studio -> Project->"Clear cache and reconfigure"