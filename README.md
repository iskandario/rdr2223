# FlashRDR2 repository

This ZIP is already arranged as a GitHub repository.

After extracting, upload the CONTENTS of this folder to the repository root.

Expected structure:

.github/
  workflows/
    flash-rdr2-windows.yml

mods/
  flash-rdr2/
    CMakeLists.txt
    FlashRDR2.ini
    INSTALL.cmd
    Install.ps1
    README.md
    src/
      Plugin.cpp

Then open GitHub -> Actions -> Build FlashRDR2 Windows x64.

When the workflow is green, download the artifact:
FlashRDR2-Windows-x64-experimental
