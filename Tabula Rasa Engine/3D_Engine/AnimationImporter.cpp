#include "AnimationImporter.h"

#include "trApp.h"
#include "trFileSystem.h"
#include "Assimp/include/anim.h"

#include "trResources.h"
#include "ResourceAnimation.h"
#include "trAnimation.h"

AnimationImporter::AnimationImporter()
{
}

AnimationImporter::~AnimationImporter()
{
}

bool AnimationImporter::Import(const char * file_path, std::string & output_file)
{
	return false;
}

UID AnimationImporter::Import(const aiAnimation* new_anim, std::string& output)
{
	static int take_count = 1;
	// Making sure the animation exists and has data inside
	if (new_anim == nullptr)
		return false;

	if (new_anim->mNumChannels == 0)
		return false;
	
	// Creating animation resource an filling its data
	ResourceAnimation* anim = (ResourceAnimation*)App->resources->CreateNewResource(Resource::Type::ANIMATION);

	std::string filename = "Take00";
	filename.append(std::to_string(take_count++));
	anim->name = filename;

	anim->ticks_per_second = new_anim->mTicksPerSecond != 0 ? new_anim->mTicksPerSecond : 24;
	anim->duration = new_anim->mDuration / anim->ticks_per_second;

	anim->num_keys = new_anim->mNumChannels;

	// Once we have the animation data we populate the animation keys with the bones' data
	anim->bone_keys = new ResourceAnimation::BoneTransformation[anim->num_keys];

	for (uint i = 0; i < new_anim->mNumChannels; ++i)
		ImportBoneTransform(new_anim->mChannels[i], anim->bone_keys[i]);

	(SaveAnimation(anim, output)) ?
		TR_LOG("Saved animation correctly in path: [%s]", output.c_str()) :
		TR_LOG("Error saving animation in path: [%s]", output.c_str());

	anim->SetExportedPath(output.c_str());

	App->animation->SetAnimationGos(anim);

	return anim->GetUID();
}

bool AnimationImporter::SaveAnimation(ResourceAnimation* anim_data, std::string & output)
{
	bool ret = false;

	// -------------- CALCULATING ANIMATION DATA SIZE --------------

	uint anim_name_size = sizeof(char)*TITLE_MAX_LENGTH; // TODO: check this to do it with a define
	uint duration_size = sizeof(anim_data->duration);
	uint ticks_size = sizeof(anim_data->ticks_per_second);
	uint num_keys_size = sizeof(anim_data->num_keys);

	uint final_size = anim_name_size + duration_size + ticks_size + num_keys_size;

	for (uint i = 0; i < anim_data->num_keys; i++)
	{
		uint id_size = sizeof(uint);
		uint bone_name_size = sizeof(char)*anim_data->bone_keys[i].bone_name.size() + 1; // TODO: check this to do it with a define

		final_size += id_size + bone_name_size;

		// Calculating positions data size
		uint pos_count_size = sizeof(anim_data->bone_keys[i].positions.count);
		uint pos_time_size = sizeof(double) * anim_data->bone_keys[i].positions.count;
		uint pos_value_size = sizeof(float) * 3 * anim_data->bone_keys[i].positions.count;

		final_size += pos_count_size + pos_time_size + pos_value_size;
		
		// Calculating rotations data size
		uint rot_count_size = sizeof(anim_data->bone_keys[i].rotations.count);
		uint rot_time_size = sizeof(double) * anim_data->bone_keys[i].rotations.count;
		uint rot_value_size = sizeof(float) * 4 * anim_data->bone_keys[i].rotations.count;

		final_size += rot_count_size + rot_time_size + rot_value_size;

		// Calculating scalings data size
		uint scale_count_size = sizeof(anim_data->bone_keys[i].scalings.count);
		uint scale_time_size = sizeof(double) * anim_data->bone_keys[i].scalings.count;
		uint scale_value_size = sizeof(float) * 3 * anim_data->bone_keys[i].scalings.count;

		final_size += scale_count_size + scale_time_size + scale_value_size;
	}

	// -------------- SAVING ANIMATION DATA --------------

	char* data = new char[final_size];
	char* cursor = data;

	// -------------- Saving animation generic data --------------

	// Saving anim name
	uint bytes = sizeof(char)*TITLE_MAX_LENGTH;

	char name[TITLE_MAX_LENGTH];
	memset(name, 0, sizeof(char) * TITLE_MAX_LENGTH);
	strcpy_s(name, TITLE_MAX_LENGTH, anim_data->name.c_str());

	memcpy(cursor, name, bytes);

	// Saving duration
	cursor += bytes;
	bytes = sizeof(anim_data->duration);
	memcpy(cursor, &anim_data->duration, bytes);

	// Saving ticks per second
	cursor += bytes;
	bytes = sizeof(anim_data->ticks_per_second);
	memcpy(cursor, &anim_data->ticks_per_second, bytes);

	// Saving num keys
	cursor += bytes;
	bytes = sizeof(anim_data->num_keys);
	memcpy(cursor, &anim_data->num_keys, bytes);

	// -------------- Saving animation bones data for each bone --------------

	for (uint i = 0; i < anim_data->num_keys; i++)
	{
		// name size
		cursor += bytes;
		bytes = sizeof(uint);
		uint name_size = anim_data->bone_keys[i].bone_name.size();
		memcpy(cursor, &name_size, bytes);

		// name
		cursor += bytes;
		bytes = sizeof(char) * name_size + 1;
		memcpy(cursor, anim_data->bone_keys[i].bone_name.c_str(), bytes);
		
		// Saving bone position data
		cursor += bytes;
		bytes = sizeof(anim_data->bone_keys[i].positions.count);
		memcpy(cursor, &anim_data->bone_keys[i].positions.count, bytes);

		cursor += bytes;
		bytes = sizeof(double) * anim_data->bone_keys[i].positions.count;
		memcpy(cursor, anim_data->bone_keys[i].positions.time, bytes);

		cursor += bytes;
		bytes = sizeof(float) * 3 * anim_data->bone_keys[i].positions.count;
		memcpy(cursor, anim_data->bone_keys[i].positions.value, bytes);

		// Saving bone rotation data
		cursor += bytes;
		bytes = sizeof(anim_data->bone_keys[i].rotations.count);
		memcpy(cursor, &anim_data->bone_keys[i].rotations.count, bytes);

		cursor += bytes;
		bytes = sizeof(double) * anim_data->bone_keys[i].rotations.count;
		memcpy(cursor, anim_data->bone_keys[i].rotations.time, bytes);

		cursor += bytes;
		bytes = sizeof(float) * 4 * anim_data->bone_keys[i].rotations.count;
		memcpy(cursor, anim_data->bone_keys[i].rotations.value, bytes);

		// Saving bone scaling data
		cursor += bytes;
		bytes = sizeof(anim_data->bone_keys[i].scalings.count);
		memcpy(cursor, &anim_data->bone_keys[i].scalings.count, bytes);

		cursor += bytes;
		bytes = sizeof(double) * anim_data->bone_keys[i].scalings.count;
		memcpy(cursor, anim_data->bone_keys[i].scalings.time, bytes);

		cursor += bytes;
		bytes = sizeof(float) * 3 * anim_data->bone_keys[i].scalings.count;
		memcpy(cursor, anim_data->bone_keys[i].scalings.value, bytes);
	}

	// -------------- Saving animation data in file --------------

	std::string tmp_str(L_ANIMATIONS_DIR);
	tmp_str.append("/");
	tmp_str.append(std::to_string(anim_data->GetUID()));
	tmp_str.append(".trAnimation"); // Adding our own format extension

	anim_data->SetExportedPath(tmp_str.c_str());

	ret = App->file_system->WriteInFile(tmp_str.c_str(), data, final_size);
	output = tmp_str;

	RELEASE_ARRAY(data);

	return ret;
}

void AnimationImporter::ImportBoneTransform(const aiNodeAnim* anim_node, ResourceAnimation::BoneTransformation& bones_transform) const
{
	// Setting up bone name
	bones_transform.bone_name = anim_node->mNodeName.C_Str();

	// Allocating memory to store bones postions
	bones_transform.positions.Init(ResourceAnimation::BoneTransformation::Key::KeyType::POSITION, anim_node->mNumPositionKeys);

	// Setting up bone positions over time
	for (uint i = 0; i < anim_node->mNumPositionKeys; i++)
	{
		bones_transform.positions.value[i * 3] = anim_node->mPositionKeys[i].mValue.x;
		bones_transform.positions.value[i * 3 + 1] = anim_node->mPositionKeys[i].mValue.y;
		bones_transform.positions.value[i * 3 + 2] = anim_node->mPositionKeys[i].mValue.z;

		bones_transform.positions.time[i] = anim_node->mPositionKeys[i].mTime;
	}

	// Allocating memory to store bones rotations
	bones_transform.rotations.Init(ResourceAnimation::BoneTransformation::Key::KeyType::ROTATION, anim_node->mNumRotationKeys);

	// Setting up bone rotations over time
	for (uint i = 0; i < anim_node->mNumRotationKeys; i++)
	{
		bones_transform.rotations.value[i * 4] = anim_node->mRotationKeys[i].mValue.x;
		bones_transform.rotations.value[i * 4 + 1] = anim_node->mRotationKeys[i].mValue.y;
		bones_transform.rotations.value[i * 4 + 2] = anim_node->mRotationKeys[i].mValue.z;
		bones_transform.rotations.value[i * 4 + 3] = anim_node->mRotationKeys[i].mValue.w;

		bones_transform.rotations.time[i] = anim_node->mRotationKeys[i].mTime;
	}

	// Allocating memory to store bones scalings
	bones_transform.scalings.Init(ResourceAnimation::BoneTransformation::Key::KeyType::SCALE, anim_node->mNumScalingKeys);

	// Setting up bone positions over time
	for (uint i = 0; i < anim_node->mNumScalingKeys; i++)
	{
		float3 scale(anim_node->mScalingKeys[i].mValue.x, 
					 anim_node->mScalingKeys[i].mValue.y, 
					 anim_node->mScalingKeys[i].mValue.z);
		
		// Forcing scale to be 1 if it's close to it to avoid floating-point errors
		// Note(victor): current epsilon is set to 1e-3f
		if (EqualsWithEpsilon(scale.x, 1.0f))
			scale.x = 1.0f;
		if (EqualsWithEpsilon(scale.y, 1.0f))
			scale.y = 1.0f;
		if (EqualsWithEpsilon(scale.z, 1.0f))
			scale.z = 1.0f;

		bones_transform.scalings.value[i * 3] = scale.x;
		bones_transform.scalings.value[i * 3 + 1] = scale.y;
		bones_transform.scalings.value[i * 3 + 2] = scale.z;

		bones_transform.scalings.time[i] = anim_node->mScalingKeys[i].mTime;
	}
}

bool AnimationImporter::EqualsWithEpsilon(float number_a, float number_b, float epsilon) const
{
	return fabs(number_a - number_b) < epsilon;
}

UID AnimationImporter::GenerateResourceFromFile(const char * file_path, UID uid_to_force)
{
	// Reading file
	char* buffer = nullptr;
	App->file_system->ReadFromFile(file_path, &buffer);

	// Checking for errors
	if (buffer == nullptr)
	{
		TR_LOG("AnimationImporter: Unable to open file...");
		return false;
	}

	ResourceAnimation* resource = (ResourceAnimation*)App->resources->CreateNewResource(Resource::Type::ANIMATION, uid_to_force);

	char* cursor = buffer;

	// Load anim name
	uint bytes = sizeof(char) * TITLE_MAX_LENGTH;
	char name[TITLE_MAX_LENGTH];
	memcpy(&name, cursor, bytes);
	resource->name = name;

	// Load duration
	cursor += bytes;
	bytes = sizeof(resource->duration);
	memcpy(&resource->duration, cursor, bytes);

	// Load ticks per second
	cursor += bytes;
	bytes = sizeof(resource->ticks_per_second);
	memcpy(&resource->ticks_per_second, cursor, bytes);

	// Load amount Bone transformations
	cursor += bytes;
	bytes = sizeof(resource->num_keys);
	memcpy(&resource->num_keys, cursor, bytes);

	resource->bone_keys = new ResourceAnimation::BoneTransformation[resource->num_keys];

	char buff[4096];
	for (uint i = 0; i < resource->num_keys; ++i)
	{
		ResourceAnimation::BoneTransformation* bone = &resource->bone_keys[i];
		uint count = 0;

		// load bone name size
		cursor += bytes;
		bytes = sizeof(count);
		memcpy(&count, cursor, bytes);

		// load bone name
		cursor += bytes;
		bytes = sizeof(char) * count + 1;
		memcpy(buff, cursor, bytes);
		bone->bone_name = buff;

		// load num_positions -------------------------------
		cursor += bytes;
		bytes = sizeof(count);
		memcpy(&count, cursor, bytes);
		bone->positions.Init(ResourceAnimation::BoneTransformation::Key::KeyType::POSITION, count);

		// load position times
		cursor += bytes;
		bytes = sizeof(double) * count;
		memcpy(bone->positions.time, cursor, bytes);

		// load position values
		cursor += bytes;
		bytes = sizeof(float) * 3 * count;
		memcpy(bone->positions.value, cursor, bytes);

		// load num rotations -------------------------------
		count = 0;
		cursor += bytes;
		bytes = sizeof(count);
		memcpy(&count, cursor, bytes);
		bone->rotations.Init(ResourceAnimation::BoneTransformation::Key::KeyType::ROTATION, count);

		// load rotation times
		cursor += bytes;
		bytes = sizeof(double) * count;
		memcpy(bone->rotations.time, cursor, bytes);

		// load rotation values
		cursor += bytes;
		bytes = sizeof(float) * 4 * count;
		memcpy(bone->rotations.value, cursor, bytes);

		// load num_scales -------------------------------
		count = 0;
		cursor += bytes;
		bytes = sizeof(count);
		memcpy(&count, cursor, bytes);
		bone->scalings.Init(ResourceAnimation::BoneTransformation::Key::KeyType::SCALE, count);

		// load position times
		cursor += bytes;
		bytes = sizeof(double) * count;
		memcpy(bone->scalings.time, cursor, bytes);

		// load position values
		cursor += bytes;
		bytes = sizeof(float) * 3 * count;
		memcpy(bone->scalings.value, cursor, bytes);
	}

	App->animation->SetAnimationGos(resource);

	RELEASE_ARRAY(buffer);

	return resource->GetUID();
}