#include "PanelHierarchy.h"
#include "ImGui/imgui.h"

#include "trApp.h"
#include "GameObject.h"
#include "trMainScene.h"
#include "trCamera3D.h"
#include "ComponentCamera.h"
#include "trWindow.h"

#include "trDefs.h"
#include <list>

#include "trEditor.h"

PanelHierarchy::PanelHierarchy() : Panel("Game Objects Hierarchy", SDL_SCANCODE_3)
{
	active = true;
}

PanelHierarchy::~PanelHierarchy()
{
}

void PanelHierarchy::Draw()
{

	ImGui::SetNextWindowPos(ImVec2(0,46.f));
	ImGui::SetNextWindowSize(ImVec2(App->window->GetWidth() * 0.21f, App->window->GetHeight() - 246.f));
	ImGui::Begin("GameObjects Hierarchy", &active, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoFocusOnAppearing);

	if (ImGui::BeginMenu("Options"))
	{
		ImGui::Text("POTATOE");

		ImGui::EndMenu();
	}

	GameObject* root = App->main_scene->GetRoot();

	if (root != nullptr) {
		std::list<GameObject*>::const_iterator it = root->childs.begin();
		while (it != root->childs.end()) {
			DrawGameObject(*it);
			it++;
		}
	}
	
	ImGui::End();
}

void PanelHierarchy::DrawGameObject(GameObject * game_object)
{
	uint tree_node_flags = 0u;

	if(game_object->childs.size() == 0)
		tree_node_flags |= ImGuiTreeNodeFlags_Leaf;

	if (game_object == App->editor->GetSelected())
		tree_node_flags |= ImGuiTreeNodeFlags_Selected;

	// TODO : solve first click error with a gameobject with childs

	if (ImGui::TreeNodeEx(game_object->GetName(), tree_node_flags))
	{
		//ItemHovered(game_object);

		if (ImGui::IsItemClicked(0)) // left click
			App->editor->SetSelected(game_object);

		if (ImGui::IsItemClicked(1))  // right click
			ImGui::OpenPopup("Options");

		if (ImGui::BeginPopup("Options"))
		{

			if (ImGui::MenuItem("Remove"))
				game_object->to_destroy = true;

			ImGui::EndPopup();
		}

		std::list<GameObject*>::const_iterator it = game_object->childs.begin();
		while (it != game_object->childs.end() && !game_object->to_destroy) {
			DrawGameObject(*it);
			it++;
		}

		ImGui::TreePop();
	}
	//else
		//ItemHovered(game_object);



}


void PanelHierarchy::ItemHovered(GameObject* hovered_go)
{
	if (ImGui::IsItemHoveredRect())
	{
		if (dragged_go && dragged_go != hovered_go)
		{
			ImGui::BeginTooltip();
			ImGui::Text("Moving %s to %s", dragged_go->GetName(), hovered_go->GetName());
			ImGui::EndTooltip();
		}

		if (ImGui::IsMouseClicked(0)) {
			App->editor->SetSelected(hovered_go);
			dragged_go = App->editor->GetSelected();
		}

		if (ImGui::IsMouseDoubleClicked(0))
			App->camera->dummy_camera->FocusOnAABB(App->camera->dummy_camera->default_aabb);

		if (dragged_go && ImGui::IsMouseReleased(0))
		{
			// TODO: when a father is released in a child it explodes
			if(dragged_go != hovered_go)
				dragged_go->SetParent(hovered_go, true);

			dragged_go = nullptr;
		}
	}
}
