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
    pirnt("\tbuild      - Builds TerraForge3D and its dependencies")
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

##############################################################################
############################### Constants ####################################
##############################################################################

ALL_COMMANDS = ["setup", "generate", "build", "clean", "format", "update", "docs", "help"]
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

VULKAN_REQUIRED_VERSION = "1.2.170.0"
VULKAN_SDK_INSTALLER_EXTENSION = "exe" if GetOS() == "nt" else ( "dmg" if GetOS() == "darwin" else "tar.gz" )
VULKAN_SDK_PLATFORM_URLS = [
    f"https://sdk.lunarg.com/sdk/download/{VULKAN_REQUIRED_VERSION}/windows/VulkanSDK-{VULKAN_REQUIRED_VERSION}-Installer.exe",
    f"https://sdk.lunarg.com/sdk/download/{VULKAN_REQUIRED_VERSION}/linux/vulkansdk-linux-x86_64-{VULKAN_REQUIRED_VERSION}.tar.gz",
    f"https://sdk.lunarg.com/sdk/download/{VULKAN_REQUIRED_VERSION}/mac/vulkansdk-macos-{VULKAN_REQUIRED_VERSION}.dmg"
]
VULKAN_SDK_URL = VULKAN_SDK_PLATFORM_URLS[0] if GetOS() == "nt" else ( VULKAN_SDK_PLATFORM_URLS[2] if GetOS() == "darwin" else VULKAN_SDK_PLATFORM_URLS[1] )
VULKAN_SDK_DIRECTORY = f"{GetProjectDirectory()}{PATH_SEPARATOR}vendor{PATH_SEPARATOR}VulkanSDK{PATH_SEPARATOR}"


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
    print("Not yet implemented")
    print("You can download AStyle manually from https://sourceforge.net/projects/astyle/ and place it in the vendor/astyle directory")

# Setup Vulkan SDK
def SetupVulkanSDK():
    if GetOS() == "nt":
        # TODO
        pass
    elif GetOS() == "darwin":
        # TODO
        pass
    elif GetOS() == "linux":
        print("Please install the Vulkan SDK and then re-run this script")
        exit(1)


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

    # Check if Vulkan Debug Libs Are Present
    print("Checking if Vulkan debug libraries are present...")
   # TODO
        
        

# Set up doxygen
def SetupDoxygen():
    print("Not yet implemented")
    print("You Can download Doxygen Manually from http://www.doxygen.nl/ and place doxygen.exe in vendor/doxygen directory")

# Sets up the environment for TerraForge3D
def Setup():

    # Mandatory steps
    SetupGitSubmodules()
    SetupPremake()
    SetupVulkan()

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
    pass

##############################################################################
############################## Build #########################################
##############################################################################

# Builds TerraForge3D and its dependencies
def Build():
    pass

##############################################################################
############################## Clean #########################################
##############################################################################

# Cleans the TerraForge3D project files and build files
def Clean():
    print("Not implemented yet")

##############################################################################
############################## Format ########################################
##############################################################################

# Formats the TerraForge3D source files (Windows Only)
def Format():
    pass
    

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
    pass

##############################################################################
######################### Main ###############################################
##############################################################################

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

    if command == "all":
        for command in ALL_COMMANDS:
            ExecuteCommand(command)
    else:
        ExecuteCommand(command)
    

if __name__ == "__main__":
    main()


