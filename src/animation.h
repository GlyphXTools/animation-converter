#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include "ChunkFile.h"
#include "types.h"
#include <string>
#include <vector>

class Animation
{
public:
#pragma pack(1)
	struct PackedQuaternion { int16_t  x, y, z, w; };
	struct PackedVector3    { uint16_t x, y, z; };
	struct Vector3          { float    x, y, z; };
#pragma pack()

	struct BoneAnimation
	{
		std::string		name;
		unsigned long	index;
		Vector3			translationOffset;
		Vector3			translationScale;
		Vector3			unknown08;
		Vector3			unknown09;
		unsigned long	unknown0a;

		short			 translationIndex;
		short			 rotationIndex;
		PackedQuaternion defaultRotation;

		std::vector<PackedQuaternion> rotations;
		std::vector<PackedVector3>    translations;
		std::vector<unsigned char>	  visibility;
		std::vector<unsigned char>	  unknown1008;
	};

	enum Type { ANIM_NONE, ANIM_EAW, ANIM_FOC };

	Animation(HANDLE hFile);

	Type getType() const;
	void write(HANDLE hFile, Type type);

private:
	void readBoneAnimation(ChunkReader& reader, BoneAnimation& bone);
	void writeBoneAnimation(ChunkWriter& writer, const BoneAnimation& bone, const Type type);

	Type						m_type;
	unsigned long				m_numFrames;
	float						m_fps;
	std::vector<BoneAnimation>	m_bones;
};

#endif