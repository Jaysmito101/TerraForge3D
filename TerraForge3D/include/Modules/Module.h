#pragma once
#include <string>
#include <unordered_map>
#include <iostream>


struct ApplicationState;

//! Module Info
/*!
   This holds essential data about the module to be shown in the module manager.
   You are supposed to fill these with relevant data in the <Module::OnLoad> function.
*/
struct ModuleInfo
{
	std::string name; /*!< The name of the module */
	std::string authorName;  /*!< The name of the author of the module */
	std::string versionString; /*!< The version of the module in the format x.x.x */
	std::string briefDescription;  /*!< A brief decription of the module (For Future Use) */
	std::string website;  /*!< The website to be shown in the module manager */
	std::string contact;  /*!< A email or url or discord link or phone number */
	std::string description;  /*!< A detailed description of the module */
	int versionMin;  /*!< Reserved For Future Use */
	int versionMax; /*!< Reserved For Future Use */
};

//! Module Info
/*!
   To make a module you are supposed to inherit this class.
   You have to Override 3 methods :
   * <OnLoad>        -> Called once when module is being loaded
   * <OnUpdate>      -> Called every Frame
   * <OnImGuiRender> -> Called for every ImGui Frame
   * <OnUnload>      -> Called at application shutdown

   You can optionally Override the <OnInstall> and <OnUninstall> methods.
*/
class Module
{
public:
	//! Module Constructor
	/*!
		For Internal Use Only
	*/
	Module(std::string uid, ApplicationState *appState);

	//! Module Destructor
	/*!
		For Internal Use Only
	*/
	~Module();

	//! Module Update Function
	/*!
		For Internal Use Only
	*/
	void Update();

	//! Module ImGui Render Function
	/*!
		For Internal Use Only
	*/
	void RenderImGui(void *imguiContext);


	//! OnInstall
	/*!
		Called just after install
	*/
	virtual void OnInstall();

	//! OnLoad
	/*!
		Called just after module is loaded
	*/
	virtual void OnLoad() = 0;

	//! OnUpdate
	/*!
		Called every frame
	*/
	virtual void OnUpdate() = 0;

	//! OnImGuiRender
	/*!
		Called for every ImGui Frame

		**Note** :  All ImGui function calls must be in this function else they wont work
	*/
	virtual void OnImGuiRender() = 0;

	//! OnUnload
	/*!
		Called just before module is being unloaded
	*/
	virtual void OnUnload() = 0;

	//! OnUninstall
	/*!
		Called just before uninstall
	*/
	virtual void OnUninstall();

public:
	std::string uid; /*!< An unique ID assigned to module when it is being loaded */
	ModuleInfo info; /*!< The details of the module */
	ApplicationState *appState; /*!< The ApplicationState allows you to control anything in TerraForge3D */
};