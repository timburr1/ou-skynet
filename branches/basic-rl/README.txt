UAlbertaBot - Readme.txt

Contact:            Dave Churchill <dave.churchill@gmail.com>
Submission Type:    Standalone dll module
File I/O:           No files are read/write during play besides BWTA load
Libraries:          BWAPI 3.6 (included in submission)
                    BWTA (included in submission)
                    BOOST 1.46.1 (not included in submission)
                    http://www.boost.org/users/history/version_1_46_1.html

Compilation Instructions:

- Open Projects\UAlbertaBot\UAlbertaBot.sln in MSVC++ 2008 Express Edition

- Edit Project Properties > C/C++ > Additional Include Directories
  - Change "C:\libraries\boost_1_46_1" to your local boost 1.46.1 directory
  
- Build solution in Release mode

The resulting Projects\Release\UAlbertaBot.dll is the dll binary to use for BWAPI