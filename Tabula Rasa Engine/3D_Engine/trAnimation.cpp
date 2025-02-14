#include "trAnimation.h"

#include "ResourceAnimation.h"
#include "GameObject.h"

#include "trDefs.h"
#include "trApp.h"
#include "trMainScene.h"
#include "trResources.h"
#include "trTimeManager.h"

#include "trInput.h" //TODO: delete this

#include "ComponentBone.h"
#include "ComponentMesh.h"

#include "ResourceBone.h"
#include "ResourceMesh.h"

#define SCALE 100 /// FBX/DAE exports set scale to 0.01
#define BLEND_TIME 1.0f

trAnimation::trAnimation()
{
	this->name = "Animation";
}

trAnimation::~trAnimation()
{}

bool trAnimation::Awake(JSON_Object* config)
{
	return true;
}

bool trAnimation::Start()
{
	if (current_anim) {
		current_anim->interpolate = true;
		current_anim->loop = true;
	}
	anim_state = AnimationState::PLAYING;

	return true;
}

// Called before quitting or switching levels
bool trAnimation::CleanUp()
{
	TR_LOG("Cleaning Animation");

	return true;
}

bool trAnimation::Update(float dt)
{
	if (current_anim == nullptr)
		return true;

	if (current_anim->anim_timer >= current_anim->duration && current_anim->duration > 0.0f)
	{
		if (current_anim->loop)
			current_anim->anim_timer = 0.0f;
		else
			anim_state = AnimationState::STOPPED;
	}

	switch (anim_state)
	{
	case AnimationState::PLAYING:
		current_anim->anim_timer += dt * current_anim->anim_speed;
		MoveAnimationForward(current_anim->anim_timer,current_anim);
		break;

	case AnimationState::PAUSED:
		break;

	case AnimationState::STOPPED:
		current_anim->anim_timer = 0.0f;
		MoveAnimationForward(current_anim->anim_timer,current_anim);
		PauseAnimation();
		break;

	case AnimationState::BLENDING:
		last_anim->anim_timer += dt * last_anim->anim_speed;
		current_anim->anim_timer += dt * current_anim->anim_speed;
		blend_timer += dt;
		float blend_percentage = blend_timer / BLEND_TIME;
		MoveAnimationForward(last_anim->anim_timer, last_anim);
		MoveAnimationForward(current_anim->anim_timer, current_anim, blend_percentage);
		if (blend_percentage >= 1.0f) {
			anim_state = PLAYING;
		}
		break;
	}


	for (uint i = 0; i < current_anim->animable_gos.size(); ++i)
	{
		ComponentBone* tmp_bone = (ComponentBone*)current_anim->animable_gos.at(i)->FindComponentByType(Component::component_type::COMPONENT_BONE);
		ResetMesh(tmp_bone);
		
	}

	for (uint i = 0; i < current_anim->animable_gos.size(); ++i)
	{
		ComponentBone* bone = (ComponentBone*)current_anim->animable_gos.at(i)->FindComponentByType(Component::component_type::COMPONENT_BONE);

		if (bone && bone->attached_mesh)
		{
			DeformMesh(bone);
		}
	}

	return true;
}

void trAnimation::SetAnimationGos(ResourceAnimation * res)
{
	Animation* animation = new Animation();
	animation->name = res->name;
	for (uint i = 0; i < res->num_keys; ++i)
		RecursiveGetAnimableGO(App->main_scene->GetRoot(), &res->bone_keys[i], animation);

	animation->duration = res->duration;

	animations.push_back(animation);
	current_anim = animations[0];
	current_anim->interpolate = true;
	current_anim->loop = true;
}

void trAnimation::RecursiveGetAnimableGO(GameObject * go, ResourceAnimation::BoneTransformation* bone_transformation, Animation* anim)
{
	if (bone_transformation->bone_name.compare(go->GetName()) == 0) 
	{
		if (!go->to_destroy) {
			anim->animable_data_map.insert(std::pair<GameObject*, ResourceAnimation::BoneTransformation*>(go, bone_transformation));
			anim->animable_gos.push_back(go);
			return;
		}
	}

	for (std::list<GameObject*>::iterator it_childs = go->childs.begin(); it_childs != go->childs.end(); ++it_childs)
		RecursiveGetAnimableGO((*it_childs), bone_transformation, anim);
}

void trAnimation::MoveAnimationForward(float time, Animation* current_animation, float blend)
{

	for (uint i = 0; i < current_animation->animable_gos.size(); ++i)
	{
		ResourceAnimation::BoneTransformation* transform = current_animation->animable_data_map.find(current_animation->animable_gos[i])->second;

		if (transform)
		{
			float3 pos, scale;
			Quat rot;

			current_animation->animable_gos[i]->GetTransform()->GetLocalPosition(&pos, &scale, &rot);

			float* prev_pos = nullptr;
			float* next_pos = nullptr;
			float time_pos_percentatge = 0.0f;

			float* prev_scale = nullptr;
			float* next_scale = nullptr;
			float time_scale_percentatge = 0.0f;

			float* prev_rot = nullptr;
			float* next_rot = nullptr;
			float time_rot_percentatge = 0.0f;

			float next_time = 0.0f;
			float prev_time = 0.0f;

			// -------- FINDING NEXT AND PREVIOUS TRANSFORMATIONS IN RELATION WITH THE GIVEN TIME (t) --------

			// Finding next and previous positions	
			if (transform->positions.count > i)
			{				
				for (uint j = 0; j < transform->positions.count; ++j)
				{
					if (prev_pos != nullptr && next_pos != nullptr) // if prev and next postions have been found we stop
					{
						float time_interval = next_time - prev_time;
						time_pos_percentatge = (time - prev_time) / time_interval;
						break;
					}

					if (time == transform->positions.time[j]) // in this case interpolation won't be done
					{
						prev_pos = &transform->positions.value[j * 3];
						next_pos = prev_pos;
						break;
					}

					if (transform->positions.time[j] > time) // prev and next postions have been found
					{
						next_time = transform->positions.time[j];
						next_pos = &transform->positions.value[j * 3];

						prev_pos = &transform->positions.value[(j * 3) - 3];
						prev_time = transform->positions.time[j - 1];
					}
				}
			}

			// Finding next and previous scalings
			if (transform->scalings.count > i)
			{
				next_time = 0.0f;
				prev_time = 0.0f;

				for (uint j = 0; j < transform->scalings.count; ++j)
				{
					if (prev_scale != nullptr && next_scale != nullptr) // if prev and next scalings have been found we stop
					{
						float time_interval = next_time - prev_time;
						time_scale_percentatge = (time - prev_time) / time_interval;
						break;
					}

					if (time == transform->scalings.time[j]) // in this case interpolation won't be done
					{
						prev_scale = &transform->scalings.value[j * 3];
						next_scale = prev_scale;
						break;
					}

					if (transform->scalings.time[j] > time) // prev and next scalings have been found
					{
						next_time = transform->scalings.time[j];
						next_scale = &transform->scalings.value[j * 3];

						prev_scale = &transform->scalings.value[(j * 3) - 3];
						prev_time = transform->scalings.time[j - 1];
					}
				}
			}
				
			// Finding next and previous rotations
			if (transform->rotations.count > i)
			{
				next_time = 0.0f;
				prev_time = 0.0f;

				for (uint j = 0; j < transform->rotations.count; ++j)
				{
					if (prev_rot != nullptr && next_rot != nullptr) // if prev and next rotations have been found we stop
					{
						float time_interval = next_time - prev_time;
						time_rot_percentatge = (time - prev_time) / time_interval; 
						break;
					}

					if (time == transform->rotations.time[j]) // in this case interpolation won't be done
					{
						prev_rot = &transform->rotations.value[j * 4];
						next_rot = prev_rot;
						break;
					}

					if (transform->rotations.time[j] > time) // prev and next rotations have been found
					{
						next_time = transform->rotations.time[j];
						next_rot = &transform->rotations.value[j * 4];

						prev_rot = &transform->rotations.value[(j * 4) - 4];
						prev_time = transform->rotations.time[j - 1];
					}
				}
			}

			// -------- INTERPOLATIONS CALCULATIONS --------

			// Interpolating positions
			if (current_animation->interpolate && prev_pos != nullptr && next_pos != nullptr && prev_pos != next_pos)
			{
				float3 prev_pos_lerp(prev_pos[0], prev_pos[1], prev_pos[2]);
				float3 next_pos_lerp(next_pos[0], next_pos[1], next_pos[2]);
				pos = float3::Lerp(prev_pos_lerp, next_pos_lerp, time_pos_percentatge);
			}
			else if (prev_pos != nullptr && (!current_animation->interpolate || prev_pos == next_pos))
				pos = float3(prev_pos[0], prev_pos[1], prev_pos[2]);

			// Interpolating scalings
			if (current_animation->interpolate && prev_scale != nullptr && next_scale != nullptr && prev_scale != next_scale)
			{
				float3 prev_scale_lerp(prev_scale[0], prev_scale[1], prev_scale[2]);
				float3 next_scale_lerp(next_scale[0], next_scale[1], next_scale[2]);
				scale = float3::Lerp(prev_scale_lerp, next_scale_lerp, time_scale_percentatge);
			}
			else if (prev_scale != nullptr && (!current_animation->interpolate || prev_scale == next_scale))
				scale = float3(prev_scale[0], prev_scale[1], prev_scale[2]);

			// Interpolating rotations
			if (current_animation->interpolate && prev_rot != nullptr && next_rot != nullptr && prev_rot != next_rot)
			{
				Quat prev_rot_lerp(prev_rot[0], prev_rot[1], prev_rot[2], prev_rot[3]);
				Quat next_rot_lerp(next_rot[0], next_rot[1], next_rot[2], next_rot[3]);
				rot = Quat::Slerp(prev_rot_lerp, next_rot_lerp, time_rot_percentatge);
			}
			else if (prev_rot != nullptr && (!current_animation->interpolate || prev_rot == next_rot))
				rot = Quat(prev_rot[0], prev_rot[1], prev_rot[2], prev_rot[3]);

			if (blend >= 1.f)
			{
				current_animation->animable_gos[i]->GetTransform()->Setup(pos, scale, rot);
			}
			else
			{
				float3 pos2, scale2;
				Quat rot2;
				last_anim->animable_gos[i]->GetTransform()->GetLocalPosition(&pos2, &scale2, &rot2);

				current_animation->animable_gos[i]->GetTransform()->Setup(float3::Lerp(pos2, pos, blend),
					float3::Lerp(scale2, scale, blend), 
					Quat::Slerp(rot2, rot, blend));
				
			}
		}

	}
}

float trAnimation::GetCurrentAnimationTime() const
{
	return current_anim->anim_timer;
}

const char* trAnimation::GetAnimationName(int index) const
{
	return animations[index]->name.c_str();
}

uint trAnimation::GetAnimationsNumber() const
{
	return (uint)animations.size();
}

trAnimation::Animation* trAnimation::GetCurrentAnimation() const
{
	return current_anim;
}

void trAnimation::SetCurrentAnimationTime(float time)
{
	current_anim->anim_timer = time;
	MoveAnimationForward(current_anim->anim_timer,current_anim);
}

void trAnimation::SetCurrentAnimation(int i)
{
	anim_state = BLENDING;
	blend_timer = 0.0f;
	last_anim = current_anim;
	current_anim = animations.at(i);
	SetCurrentAnimationTime(0.0f);
}

void trAnimation::CleanAnimableGOS()
{
	for (uint i = 0; i < animations.size(); ++i)
	{
		animations.at(i)->animable_gos.clear();
		animations.at(i)->animable_data_map.clear();
	}

	for (std::vector<Animation*>::iterator it = animations.begin(); it != animations.end(); ++it)
		RELEASE(*it);

	animations.clear();
	current_anim = nullptr;
}

void trAnimation::PlayAnimation()
{
	anim_state = AnimationState::PLAYING;
}

void trAnimation::PauseAnimation()
{
	anim_state = AnimationState::PAUSED;
}

void trAnimation::StopAnimation()
{
	anim_state = AnimationState::STOPPED;
}

void trAnimation::StepBackwards()
{
	if (current_anim->anim_timer > 0.0f)
	{
		current_anim->anim_timer -= App->time_manager->GetRealTimeDt() * current_anim->anim_speed;

		if (current_anim->anim_timer < 0.0f)
			current_anim->anim_timer = 0.0f;
		else
			MoveAnimationForward(current_anim->anim_timer,current_anim);
		
		PauseAnimation();
	}
}

void trAnimation::StepForward()
{
	if (current_anim->anim_timer < current_anim->duration)
	{
		current_anim->anim_timer += App->time_manager->GetRealTimeDt() * current_anim->anim_speed;

		if (current_anim->anim_timer > current_anim->duration)
			current_anim->anim_timer = 0.0f;
		else
			MoveAnimationForward(current_anim->anim_timer,current_anim);

		PauseAnimation();
	}
}

void trAnimation::DeformMesh(ComponentBone* component_bone)
{
	ComponentMesh* mesh = component_bone->attached_mesh;

	if (mesh != nullptr)
	{
		const ResourceBone* rbone = (const ResourceBone*)component_bone->GetResource();
		const ResourceMesh* roriginal = (const ResourceMesh*)mesh->GetResource();
		ResourceMesh* tmp_mesh = (ResourceMesh*)mesh->GetResource();
		ResourceMesh* rmesh = (ResourceMesh*)tmp_mesh->deformable;

		float4x4 trans = component_bone->GetEmbeddedObject()->GetTransform()->GetMatrix();
		trans = trans * component_bone->attached_mesh->GetEmbeddedObject()->GetTransform()->GetLocal().Inverted();

		trans = trans * rbone->offset_matrix;

		for (uint i = 0; i < rbone->bone_weights_size; ++i)
		{
			uint index = rbone->bone_weights_indices[i];
			float3 original(&roriginal->vertices[index * 3]);

			float3 vertex = trans.TransformPos(original);

			rmesh->vertices[index * 3] += vertex.x * rbone->bone_weights[i] * SCALE;
			rmesh->vertices[index * 3 + 1] += vertex.y * rbone->bone_weights[i] * SCALE;
			rmesh->vertices[index * 3 + 2] += vertex.z * rbone->bone_weights[i] * SCALE;
		}
	}
}

void trAnimation::ResetMesh(ComponentBone * component_bone)
{
	ResourceBone* rbone = (ResourceBone*)component_bone->GetResource();
	ResourceMesh* original = nullptr;
	if (rbone)
		original = (ResourceMesh*)App->resources->Get(rbone->mesh_uid);

	if(original)
		memset(original->deformable->vertices, 0, original->vertex_size * sizeof(float));
}
