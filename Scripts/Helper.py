##############################################################################
# This file is part of TerraForge3D (https://github.com/TerraForge3D/)
#
# Maintainers:
# - Jaysmito Mukherjee ( jaysmito101@gmail.com )
#
# This is licensed under the MIT license
##############################################################################

##############################################################################
######################## Imports #############################################
##############################################################################

import os
import sys
import time
import urllib
import urllib.request
import zipfile
import tarfile
import subprocess
from pathlib import Path

##############################################################################
######################## Utility Functions ###################################
##############################################################################

# Prints the usage of this script as well as some error messages if needed
def PrintUsage(errorMessage=None, exit=True):
    if errorMessage:
        print("ERROR : " + errorMessage + "\n")
    print("Usage:")
    print("\tpython3 Helper.py [command]")
    print("\nCommands:")
    print("\tsetup      - Sets up the environment for TerraForge3D ( default )")
    print("\tgenerate   - Generates the TerraForge3D project files")
    print("\tbuild      - Builds TerraForge3D and its dependencies")
    print("\tclean      - Cleans the TerraForge3D project files and build files")
    print("\tformat     - Formats the TerraForge3D source files (Windows Only)")
    print("\tupdate     - Updates git submodules ...")
    print("\tdocs       - Generates the TerraForge3D documentation")
    print("\tall        - Runs all the commands")
    print("\thelp       - Prints this message")

    if exit:
        sys.exit( 1 if errorMessage else 0 )

# Check if python version version is 3.4 or greater
def CheckPythonVersion():
    if sys.version_info < (3, 4):
        PrintUsage("Python version must be 3.4 or greater")

# Read a file and return its contents
def ReadTextFile(filePath):
    with open(filePath, "r") as file:
        return file.read()

# Write a file
def WriteTextFile(filePath, text):
    with open(filePath, "w") as file:
        file.write(text)

# Get Operation System
def GetOS():
    return os.name

# Checks if a file exists or not
def FileExists(filePath):
    return os.path.isfile(filePath)

# Check if a folder exists or not
def FolderExists(folderPath):
    return os.path.isdir(folderPath)

# Returns current working directory
def GetCurrentWorkingDirectory():
    return os.getcwd()

# Return project directory
def GetProjectDirectory():
    scriptPath = os.path.dirname(os.path.realpath(__file__))
    return Path(scriptPath).parent.absolute()

# Go to the project directory
def FixCurrentDirectory():
    if GetCurrentWorkingDirectory()  != GetProjectDirectory():
        os.chdir(GetProjectDirectory())

# Download a file from a url
def DownloadFile(url, filePath):
    print("Downloading file from " + url + " to " + filePath)
    urllib.request.urlretrieve(url, filePath)

# Make a directory
def MakeDirectory(directoryPath):
    if not FolderExists(directoryPath):
        os.makedirs(directoryPath)

# Exrtacts a zip file
def ExtractZip(archivePath, destinationPath):
    with zipfile.ZipFile(archivePath, 'r') as zipRef:
        zipRef.extractall(destinationPath)

# Extracts a tar.gz file
def ExtractTarGz(archivePath, destinationPath):
    with tarfile.open(archivePath, 'r:gz') as tarRef:
        tarRef.extractall(destinationPath)

# Extracts a zip or tar.gz file
def ExtractArchive(archivePath, destinationPath):
    if archivePath.endswith(".zip"):
        ExtractZip(archivePath, destinationPath)
    elif archivePath.endswith(".tar.gz"):
        ExtractTarGz(archivePath, destinationPath)
    else:
        print("Cannot extract archive " + archivePath)
        exit(1)

# Check if registry key exists for windows only
def CheckRegistryKeyExists(key):
    if GetOS() == "nt":
        import winreg
        try:
            winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, key)
            return True
        except WindowsError:
            return False
    else:
        return False

# Copu a directory recursively
def CopyDirectory(src, dest, ignore=None):
    if os.path.isdir(src):
        if not os.path.isdir(dest):
            os.makedirs(dest)
        files = os.listdir(src)
        if ignore is not None:
            ignored = ignore(src, files)
        else:
            ignored = set()
        for f in files:
            if f not in ignored:
                recursive_overwrite(os.path.join(src, f), 
                                    os.path.join(dest, f), 
                                    ignore)
    else:
        shutil.copyfile(src, dest)

##############################################################################
############################### Constants ####################################
##############################################################################

ALL_COMMANDS = ["setup", "generate", "build", "clean", "format", "update", "docs", "copydir", "help"]
PATH_SEPARATOR = os.sep
EXECUTABLE_EXTENSION = ".exe" if GetOS() == "nt" else ""
ARCHIVE_EXTENSION = "zip" if GetOS() == "nt" else "tar.gz"

PREMAKE_DIRECTORY = f"{GetProjectDirectory()}{PATH_SEPARATOR}vendor{PATH_SEPARATOR}premake{PATH_SEPARATOR}"
PREMAKE_EXECUTABLE_PATH = f"{PREMAKE_DIRECTORY}premake5{EXECUTABLE_EXTENSION}"
PREMAKE_VERSION = "5.0.0-beta1"
PREMAKE_OS_NAME = "windows" if GetOS() == "nt" else ( "macosx" if GetOS() == "darwin" else "linux" )
PREMAKE_ARCHIVE_EXTENSION = ARCHIVE_EXTENSION
PREMAKE_ARCHIVE_URL = f"https://github.com/premake/premake-core/releases/download/v{PREMAKE_VERSION}/premake-{PREMAKE_VERSION}-{PREMAKE_OS_NAME}.{PREMAKE_ARCHIVE_EXTENSION}"
PREMAKE_LISCENSE_URL = f"https://raw.githubusercontent.com/premake/premake-core/master/LICENSE.txt"

VULKAN_REQUIRED_VERSION = "1.3.204.1"
VULKAN_SDK_INSTALLER_EXTENSION = "exe" if GetOS() == "nt" else ( "dmg" if GetOS() == "darwin" else "tar.gz" )
VULKAN_SDK_INSTALLER_PLATFORM_URLS = [
    f"https://sdk.lunarg.com/sdk/download/{VULKAN_REQUIRED_VERSION}/windows/VulkanSDK-{VULKAN_REQUIRED_VERSION}-Installer.exe",
    f"https://sdk.lunarg.com/sdk/download/{VULKAN_REQUIRED_VERSION}/linux/vulkansdk-linux-x86_64-{VULKAN_REQUIRED_VERSION}.tar.gz",
    f"https://sdk.lunarg.com/sdk/download/{VULKAN_REQUIRED_VERSION}/mac/vulkansdk-macos-{VULKAN_REQUIRED_VERSION}.dmg"
]
VULKAN_SDK_INSTALLER_URL = VULKAN_SDK_INSTALLER_PLATFORM_URLS[0] if GetOS() == "nt" else ( VULKAN_SDK_INSTALLER_PLATFORM_URLS[2] if GetOS() == "darwin" else VULKAN_SDK_INSTALLER_PLATFORM_URLS[1] )
VULKAN_SDK_DIRECTORY = f"{GetProjectDirectory()}{PATH_SEPARATOR}vendor{PATH_SEPARATOR}VulkanSDK{PATH_SEPARATOR}"

VC_RUNTIME_INSTALLER_URL = "https://aka.ms/vs/17/release/vc_redist.x64.exe"

ASTYLE_DIRECTORY = f"{GetProjectDirectory()}{PATH_SEPARATOR}vendor{PATH_SEPARATOR}astyle{PATH_SEPARATOR}"
ASTYLE_URL = "https://downloads.sourceforge.net/project/astyle/astyle/astyle%203.1/AStyle_3.1_windows.zip?ts=gAAAAABiUHLIzztXmAjVoFZx_mGeriR1Bk5MMhgocea1Dod-nkmVrVkk5OoHgb_cWuYJZVlVm8mRjTxjjCOgLXKHXXqGAP-nPg%3D%3D&r=https%3A%2F%2Fsourceforge.net%2Fprojects%2Fastyle%2Ffiles%2Flatest%2Fdownload"
ASTYLE_EXECUTABLE_PATH = f"{ASTYLE_DIRECTORY}AStyle{PATH_SEPARATOR}bin{PATH_SEPARATOR}AStyle.exe"

BUILD_CONFIGURATIONS = ["Debug", "Release"]
STATE = {}

##############################################################################
####################### State Handler Functions ##############################
##############################################################################

# Function to load the state from the file
# State File Format:
# KEY=VALUE
#
# Note : The content is case insensitive
def LoadState(filePath):
    global STATE
    if not os.path.exists(filePath):
        return
    with open(filePath, "r") as file:
        for line in file:
            line = line.strip()
            if line.startswith("#") or line == "":
                continue
            key, value = line.split("=")
            STATE[key.upper()] = value.upper()

# Function to save the state to the file
def SaveState(filePath):
    global STATE
    with open(filePath, "w") as file:
        for key in STATE:
            file.write(f"{key}={STATE[key]}\n")

##############################################################################
############################## Setup #########################################
##############################################################################

# Sets up the git submodules
def SetupGitSubmodules():
    print("Setting up git submodules...")
    os.system("git submodule update --init --recursive")

# Setup Premake
def SetupPremake():
    print("Checking if premake is installed...")
    if FileExists(PREMAKE_EXECUTABLE_PATH):
        print("Premake already installed")
    else:
        print("Premake not installed")
        print("Setting up premake...")
        print("Downloading premake...")
        MakeDirectory(PREMAKE_DIRECTORY)
        DownloadFile(PREMAKE_ARCHIVE_URL, PREMAKE_DIRECTORY + "premake-archived." + PREMAKE_ARCHIVE_EXTENSION)
        DownloadFile(PREMAKE_LISCENSE_URL, PREMAKE_DIRECTORY + "LICENSE.txt")
        print("Extracting premake...")
        ExtractArchive(PREMAKE_DIRECTORY + "premake-archived." + PREMAKE_ARCHIVE_EXTENSION, PREMAKE_DIRECTORY)
        print("Cleaning premake archive...")
        os.remove(PREMAKE_DIRECTORY + "premake-archived." + PREMAKE_ARCHIVE_EXTENSION)
        print("Premake setup complete")

# Setup AStyle
def SetupAStyle():
    if not GetOS() == "nt":
        return
    print("Checking if astyle is installed...")
    if FileExists(ASTYLE_DIRECTORY + "astyle" + EXECUTABLE_EXTENSION):
        print("AStyle already installed")
    else:
        print("AStyle is not installed")
        print("Setting up astyle...")
        # Mak sure the directory exists
        MakeDirectory(ASTYLE_DIRECTORY)
        # Download Asytle
        DownloadFile(ASTYLE_URL, ASTYLE_DIRECTORY + "astyle-archived." + ARCHIVE_EXTENSION)
        # Extract Asytle
        ExtractArchive(ASTYLE_DIRECTORY + "astyle-archived." + ARCHIVE_EXTENSION, ASTYLE_DIRECTORY)
        # Clean Asytle Archive
        os.remove(ASTYLE_DIRECTORY + "astyle-archived." + ARCHIVE_EXTENSION)


# Setup Vulkan SDK
def SetupVulkanSDK():
    # Download Vulkan SDK installer
    DownloadFile(VULKAN_SDK_INSTALLER_URL, VULKAN_SDK_DIRECTORY + "VulkanSDK-Installer." + VULKAN_SDK_INSTALLER_EXTENSION)
    if GetOS() == "nt":
        print("The Installer will now be launched.\nPlease follow the instructions and install the SDK.\nRun the setup script again after the SDK has been installed.")
        # Launch the installer
        os.system(VULKAN_SDK_DIRECTORY + "VulkanSDK-Installer." + VULKAN_SDK_INSTALLER_EXTENSION)
        # Clean up the installer
        os.remove(VULKAN_SDK_DIRECTORY + "VulkanSDK-Installer." + VULKAN_SDK_INSTALLER_EXTENSION)
        exit(0)
    elif GetOS() == "darwin":
        print("The Required Vulkan SDK Installer has been downloaded to " + VULKAN_SDK_DIRECTORY + "VulkanSDK-Installer." + VULKAN_SDK_INSTALLER_EXTENSION + ".\nPlease manually install it and run this script again.")
        exit(0)
    elif GetOS() == "linux":
        print("The Required Vulkan SDK has been downloaded to " + VULKAN_SDK_DIRECTORY + "VulkanSDK-Installer." + VULKAN_SDK_INSTALLER_EXTENSION + ".\nPlease install it yourself and re run this script.")
        exit(0)


# Sets up Vulkan
def SetupVulkan():
    # Setup Vulkan SDK
    print("Checking for Vulkan SDK...")
    vulkanSDK = os.environ.get("VULKAN_SDK")
    if vulkanSDK is None:
        print("Vulkan SDK not found")
        print("Setting up Vulkan SDK...")
        SetupVulkanSDK()
        print("Vulkan SDK setup complete")
    else:
        print("Vulkan SDK found")
    
    # Check if Vulkan SDK Version is correct
    print("Checking Vulkan SDK version...")
    if VULKAN_REQUIRED_VERSION not in vulkanSDK:
        print("Vulkan SDK version is incorrect")
        print("Setting up Vulkan SDK...")
        SetupVulkanSDK()
        print("Vulkan SDK setup complete")
    else:
        print("Vulkan SDK version is correct")

    print("Correcting Vulkan SDK path located at " + vulkanSDK)


# Set up doxygen
def SetupDoxygen():
    print("Not yet implemented")
    print("You Can download Doxygen Manually from http://www.doxygen.nl/ and place doxygen.exe in vendor/doxygen directory")

# Setup Redistributable binaries
def SetupRedistributableBinaries():
    # Make sure the directory exists
    MakeDirectory(f"{GetProjectDirectory()}{PATH_SEPARATOR}Binaries{PATH_SEPARATOR}Data{PATH_SEPARATOR}VCRuntime{PATH_SEPARATOR}")
    # Download the redistributable binaries is they don't exist
    if not FileExists(f"{GetProjectDirectory()}{PATH_SEPARATOR}Binaries{PATH_SEPARATOR}Data{PATH_SEPARATOR}VCRuntime{PATH_SEPARATOR}vc_redist.x64.exe"):
        DownloadFile(VC_RUNTIME_INSTALLER_URL, f"{GetProjectDirectory()}{PATH_SEPARATOR}Binaries{PATH_SEPARATOR}Data{PATH_SEPARATOR}VCRuntime{PATH_SEPARATOR}vc_redist.x64.exe")

# Sets up the environment for TerraForge3D
def Setup():

    # Mandatory steps
    SetupGitSubmodules()
    SetupPremake()
    SetupVulkan()
    SetupRedistributableBinaries()

    # Optional steps
    SetupDoxygen()
    if GetOS() == "nt":
        SetupAStyle()

    # Create a file to cache that setup has been run
    WriteTextFile(".helpercache", "SETUP=1")

##############################################################################
####################### Generate Project Files ###############################
##############################################################################

# Generates the TerraForge3D project files
def GenerateProjectFiles():
    if not FileExists(PREMAKE_EXECUTABLE_PATH):
        print("Premake not found")
        SetupPremake()
    else:
        # Select Default project type depending upon OS
        # Windows - Visual Studio 2022
        # Linux   - Makefiles
        # MacOS   - Makefiles
        premakeProjectType = "vs2022" if GetOS() == "nt" else ("gmake2" if GetOS() == "darwin" else "gmake2")
        # Take project type from arument if provided [Should not be used]
        if len(sys.argv) > 2 and sys.argv[1] == "generate":
            premakeProjectType = sys.argv[2]
        # Call Premake5 to generate project files
        subprocess.run([PREMAKE_EXECUTABLE_PATH, premakeProjectType])

##############################################################################
############################## Build #########################################
##############################################################################

# Builds TerraForge3D and its dependencies on Windows
def BuildOnWindows(buildConfiguration):
    # Setup visual studio developer environment
    # NOTE : This scripts assumes you have Visual Studio 2022 Community Edition Installed
    os.system("call \"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat\" x64  && " + f"msbuild /m /p:PlatformTarget=x64 /p:Configuration={buildConfiguration} TerraForge3D.sln")
    # I dont know why this is required but it doesnt work without this
    # os.system("SET Platform=")
    # Build TerraForge3D
    # os.system(f"msbuild /m /p:PlatformTarget=x64 /p:Configuration={buildConfiguration} TerraForge3D.sln")

# Builds TerraForge3D and its dependencies on Linux
def BuildOnLinux(buildConfiguration):
    os.system(f"make config={buildConfiguration}")

# Builds TerraForge3D and its dependencies on MacOS
def BuildOnMacOS(buildConfiguration):
    os.system(f"make config={buildConfiguration}")

# Builds TerraForge3D and its dependencies
def Build():
    # Default Build Configuration - DebugVkCompute
    buildConfiguration = BUILD_CONFIGURATIONS[0]
    # Use Build Configuration passed as argument if its valid
    if len(sys.argv) > 2 and sys.argv[1] == "build":
        if sys.argv[2] in BUILD_CONFIGURATIONS:
            buildConfiguration = sys.argv[2]
    # Call the dedicated function depending upon OS
    if GetOS() == "nt":
        BuildOnWindows(buildConfiguration)
    elif GetOS() == "darwin":
        BuildOnMacOS(buildConfiguration)
    elif GetOS() == "linux":
        BuildOnLinux(buildConfiguration)

##############################################################################
############################## Clean #########################################
##############################################################################

# Cleans the TerraForge3D project files and build files
def Clean():
    if GetOS() == "nt":
        print("Not implemented yet")
    else:
        os.system("make clean")

##############################################################################
############################## Format ########################################
##############################################################################

# Formats the TerraForge3D source files (Windows Only)
def Format():
    if not FileExists(f"{GetProjectDirectory()}{PATH_SEPARATOR}vendor{PATH_SEPARATOR}astyle{PATH_SEPARATOR}astyle.exe"):
        print("AStyle not found")
        SetupAStyle()
    else:
        # Call AStyle with the desired arguments
        subprocess.run([ASTYLE_EXECUTABLE_PATH, "--style=allman", "--indent=tab", "--indent-switches", "--break-blocks=all", "--pad-comma", "--delete-empty-lines", "--align-pointer=name", "--align-reference=name", "--break-closing-braces", "--break-one-line-headers", "--add-braces", "--recursive", f"{GetProjectDirectory()}{PATH_SEPARATOR}TerraForge3D{PATH_SEPARATOR}*.c,*.cpp,*.h,*.hpp"])
        # Delete backup files generated by AStyle
        subprocess.run(["del", "/s", "/q", "*.orig"])
    

##############################################################################
############################## Update ########################################
##############################################################################

# Updates git submodules
def Update():
    os.system("git submodule update --init --recursive")

##############################################################################
####################### Generate Documentation ###############################
##############################################################################

# Generates the TerraForge3D documentation
def GenerateDocs():
    if not FileExists(f"{GetProjectDirectory()}{PATH_SEPARATOR}vendor{PATH_SEPARATOR}doxygen{PATH_SEPARATOR}doxygen" + EXECUTABLE_EXTENSION):
        print("Doxygen not found")
        SetupDoxygen()
    else:
        # Call Doxygen
        subprocess.run([f"{GetProjectDirectory()}{PATH_SEPARATOR}vendor{PATH_SEPARATOR}doxygen{PATH_SEPARATOR}doxygen" + EXECUTABLE_EXTENSION])
        print("Docs Generated to Docs/html folder")

##############################################################################
######################### Main ###############################################
##############################################################################

# Execute a command
def ExecuteCommand(command):
    if command == "setup":
        Setup()
    elif command == "generate":
        GenerateProjectFiles()
    elif command == "build":
        Build()
    elif command == "clean":
        Clean()
    elif command == "format":
        Format()
    elif command == "update":
        Update()
    elif command == "docs":
        GenerateDocs()
    elif command == "help":
        PrintUsage(exit=True)
    else:
        PrintUsage("Unknown command '" + command + "'", False)
    

# The main function of this script
def main():

    # Check if python version is 3.4 or greater
    CheckPythonVersion()

    # Go to currect directory
    FixCurrentDirectory()

    command = sys.argv[1] if len(sys.argv) > 1 else "setup" # Default command is setup

    # If setup has not been run at all run it first
    if not FileExists(".helpercache"):
        print("Setup has not been run yet. Running setup...")
        ExecuteCommand("setup")
    
    LoadState(".helpercache")

    if command == "all":
        for command in ALL_COMMANDS:
            ExecuteCommand(command)
    else:
        ExecuteCommand(command)
    
    SaveState(".helpercache")
    

if __name__ == "__main__":
    main()


