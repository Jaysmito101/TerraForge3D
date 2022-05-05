#pragma once
#include "Base/Core/Core.hpp"

namespace TerraForge3D
{

	namespace UI
	{
		enum MenuItemType
		{
			MenuItemType_Item,
			MenuItemType_SubMenu
		};

		enum MenuItemUse
		{
			MenuItemUse_Button,
			MenuItemUse_Toggle
		};

		class MenuItem
		{
		public:
			MenuItem(std::string name, std::string path, MenuItemUse use = MenuItemUse_Button, MenuItemType type = MenuItemType_Item);
			~MenuItem();

			bool Begin();
			void End();
			void Show();

			MenuItem* AddItem(MenuItemUse use, std::string subpath, std::function<void(MenuItem*)> callback);


			inline std::string GetPath() { return this->path; }
			inline std::string GetName() { return this->name; }
			inline std::string GetShortcut() { return this->shortcut; }
			inline std::string GetTooltip() { return this->tooltip; }
			inline bool IsEnabled() { return this->enabled; }
			inline bool IsSelected() { return this->selected; }
			inline bool GetToggleState() { TF3D_ASSERT(use = MenuItemUse_Toggle, "Cannot get toggle state without mode being toggle"); return this->toggleState; }
			
			inline MenuItem* SetCallback(std::function<void(MenuItem*)> callback) { TF3D_ASSERT(callback, "Callback is null");	this->callback = callback; return this; }
			inline MenuItem* SetTooltip(std::string tooltip) { this->tooltip = tooltip;  return this;};
			inline MenuItem* SetShortcut(std::string shortcut) { this->shortcut = shortcut;  return this;};
			inline MenuItem* SetEnabled(bool enabled) { this->enabled = enabled; return this; }

		private:
			MenuItemType type = MenuItemType_Item;
			MenuItemUse use = MenuItemUse_Button;
			std::unordered_map<std::string, MenuItem*> subMenus;
			std::function<void(MenuItem*)> callback;
			std::string path = "";
			std::string name = "";
			std::string tooltip = "";
			std::string shortcut = "";
			bool selected = false;
			bool toggleState = false;
			bool enabled = true;
			std::string uid = UUID::Generate().ToString();
		};

		class Menu
		{
		public:
			Menu() = default;
			~Menu();

			MenuItem* Register(std::string path, std::function<void(MenuItem*)> callback, MenuItemUse use = MenuItemUse_Button);
			void Deregister(std::string path);

			void Show();

			private:
				std::string uid = UUID::Generate().ToString();
				std::unordered_map<std::string, MenuItem*> subMenus;
		};

	}

}
