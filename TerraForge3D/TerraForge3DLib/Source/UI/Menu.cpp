#include "UI/Menu.hpp"


#include "imgui/imgui.h"


static void DefaultCallback(TerraForge3D::UI::MenuItem* context)
{
	TF3D_LOG("Menu Item : {0} clicked", context->GetPath());
}

namespace TerraForge3D
{

	namespace UI
	{

		MenuItem::MenuItem(std::string name, std::string path, MenuItemUse use, MenuItemType type)
		{
			this->path = path;
			this->name = name;
			this->use = use;
			this->type = type;
			this->tooltip = (type == MenuItemType_Item ?  "Menu Item" : "Sub Menu");
			this->callback = DefaultCallback;
			subMenus.clear();
		}

		MenuItem::~MenuItem()
		{
			for (auto& [k, v] : subMenus)
			{
				delete v;
			}
			subMenus.clear();
		}

		bool MenuItem::Begin()
		{
			if (type == MenuItemType_SubMenu)
			{
				return ImGui::BeginMenu(name.data(), enabled);
			}
			else
			{
				if (use == MenuItemUse_Button)
				{
					if (ImGui::MenuItem(name.data(), shortcut.data(), &selected, enabled))
					{
						if (callback)
							callback(this);
					}
				}
				else if (use == MenuItemUse_Toggle)
				{
					if (ImGui::Checkbox(name.data(), &toggleState))
					{
						if (callback)
							callback(this);
					}
				}
				return false;
			}
		}

		void MenuItem::End()
		{
			if (type == MenuItemType_SubMenu)
			{
				ImGui::EndMenu();
			}
		}

		void MenuItem::Show()
		{
			ImGui::PushID(uid.data());
			if (Begin())
			{
				// This will be executed only in case of type == MenuItemType_SubMenu
				for (auto& [k, v] : subMenus)
				{
					v->Show();
				}
				End();
			}
			ImGui::PopID();
		}

		MenuItem* MenuItem::AddItem(MenuItemUse us, std::string subpath, std::function<void(MenuItem*)> cb)
		{
			TF3D_ASSERT(type == MenuItemType_SubMenu, "Cannot add item to a item (type must be SubMenu)");
			std::string subpathBase = subpath.substr(0, subpath.find("/"));
			std::string subpathRest = subpath.substr(subpath.find("/") + 1);

			if (subpathRest.find("/") == std::string::npos)
			{
				if (subpath == subpathBase)
				{
					if (subMenus.find(subpathBase) != subMenus.end())
						delete subMenus[subpathBase];
					subMenus[subpathBase] = new MenuItem(subpathBase, path + "/" + subpathBase, us, MenuItemType_Item);
					subMenus[subpathBase]->SetCallback(cb);
					return subMenus[subpathBase];
				}
				else
				{
					if (subMenus.find(subpathBase) == subMenus.end())
						subMenus[subpathBase] = new MenuItem(subpathBase, path + "/" + subpathBase, MenuItemUse_Button, MenuItemType_SubMenu);
					return subMenus[subpathBase]->AddItem(us, subpathRest, cb);
				}
			}
			else
			{
				if (subMenus.find(subpathBase) == subMenus.end())
					subMenus[subpathBase] = new MenuItem(subpathBase, path + "/" + subpathBase, MenuItemUse_Button, MenuItemType_SubMenu);
				else if (subMenus[subpathBase]->type == MenuItemType_Item)
				{
					delete subMenus[subpathBase];
					subMenus[subpathBase] = new MenuItem(subpathBase, path + "/" + subpathBase, MenuItemUse_Button, MenuItemType_SubMenu);
				}
				return subMenus[subpathBase]->AddItem(us, subpathRest, cb);
			}
		}


		Menu::~Menu()
		{
			for (auto& [k, v] : subMenus)
			{
				delete v;
			}
			subMenus.clear();
		}
		
		MenuItem* Menu::Register(std::string path, std::function<void(MenuItem*)> callback, MenuItemUse use)
		{
			std::string baseMenu = path.substr(0, path.find("/"));
			std::string restPath = path.substr(path.find("/") + 1);

			if (restPath.size() > 0)
			{
				if (subMenus.find(baseMenu) == subMenus.end())
					subMenus[baseMenu] = new MenuItem(baseMenu, baseMenu, MenuItemUse_Button, MenuItemType_SubMenu);
				return subMenus[baseMenu]->AddItem(use, restPath, callback);
			}
			return nullptr;
		}

		void Menu::Deregister(std::string path)
		{
			// TODO : Implement
		}

		void Menu::Show()
		{
			if (ImGui::BeginMainMenuBar())
			{
				for (auto& [k, v] : subMenus)
				{
					v->Show();
				}
				ImGui::EndMainMenuBar();
			}
		}
	}

}