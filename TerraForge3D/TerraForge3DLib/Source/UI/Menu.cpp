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
				TF3D_SAFE_DELETE(v);
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
					if(togglePTR)
						toggleState= *togglePTR;
					/*
					if (ImGui::Checkbox(name.data(), &toggleState))
					{
						if (callback)
							callback(this);
					}
					*/
					if (ImGui::MenuItem(name.data(), shortcut.data(), toggleState))
					{
						toggleState = !toggleState;
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

		MenuItem* MenuItem::AddItem(MenuItemUse us, std::vector<std::string> subpath, std::function<void(MenuItem*)> cb)
		{
			TF3D_ASSERT(type == MenuItemType_SubMenu, "Cannot add item to a item (type must be SubMenu)");
			std::string subpathBase = subpath[0];
			subpath.erase(subpath.begin());
			std::vector<std::string> subpathRest = subpath;

			if (subpathRest.size() == 0)
			{
				if (subMenus.find(subpathBase) != subMenus.end())
					delete subMenus[subpathBase];
				subMenus[subpathBase] = new MenuItem(subpathBase, path + "/" + subpathBase, us, MenuItemType_Item);
				subMenus[subpathBase]->SetCallback(cb);
				return subMenus[subpathBase];
			}
			else
			{
				if (subMenus.find(subpathBase) != subMenus.end() && subMenus[subpathBase]->type == MenuItemType_Item)
				{
					TF3D_SAFE_DELETE(subMenus[subpathBase]);
					subMenus[subpathBase] = new MenuItem(subpathBase, path + "/" + subpathBase, MenuItemUse_Button, MenuItemType_SubMenu);
				}
				else if(subMenus.find(subpathBase) == subMenus.end())
				{
					subMenus[subpathBase] = new MenuItem(subpathBase, path + "/" + subpathBase, MenuItemUse_Button, MenuItemType_SubMenu);
				}
				return subMenus[subpathBase]->AddItem(us, subpathRest, cb);
			}
		}

		bool MenuItem::RemoveItem(std::vector<std::string> subpath)
		{
			std::string subpathBase = subpath[0];
			subpath.erase(subpath.begin());
			std::vector<std::string> subpathRest = subpath;

			if (subMenus.find(subpathBase) == subMenus.end())
				return false;

			if (subpathRest.size() == 0)
			{
				TF3D_SAFE_DELETE(subMenus[subpathBase]);
				subMenus.erase(subMenus.find(subpathBase));
				return true;
			}
			else
			{
				return subMenus[subpathBase]->RemoveItem(subpathRest);
			}
		}


		Menu::~Menu()
		{
			for (auto& [k, v] : subMenus)
			{
				TF3D_SAFE_DELETE(v);
			}
			subMenus.clear();
		}
		
		MenuItem* Menu::Register(std::vector<std::string> path, std::function<void(MenuItem*)> callback, MenuItemUse use)
		{
			std::string baseMenu = path[0];
			path.erase(path.begin());
			std::vector<std::string> restPath = path;

			if (restPath.size() > 0)
			{
				if (subMenus.find(baseMenu) == subMenus.end())
					subMenus[baseMenu] = new MenuItem(baseMenu, baseMenu, MenuItemUse_Button, MenuItemType_SubMenu);
				return subMenus[baseMenu]->AddItem(use, restPath, callback);
			}
			return nullptr;
		}

		void Menu::Deregister(std::vector<std::string> path)
		{
			// TODO : Implement
		}

		void Menu::Show()
		{
			if(!isEnabled)
				return;

			bool tmp = false;
			if (isMainMenu)
				tmp = ImGui::BeginMainMenuBar();
			else
				tmp = ImGui::BeginMenuBar();

			for (auto& [k, v] : subMenus)
			{
				v->Show();
			}
			if (tmp)
			{
				if (isMainMenu)
					ImGui::EndMainMenuBar();
				else
					ImGui::EndMenuBar();
			}
			
		}
	}

}