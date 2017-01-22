#include "animation.h"
#include "exceptions.h"

using namespace std;

static void Verify(int expr)
{
	if (!expr)
	{
		throw BadFileException();
	}
}

//
// Animation class
//
Animation::Type Animation::getType() const
{
	return m_type;
}

void Animation::writeBoneAnimation(ChunkWriter& writer, const BoneAnimation& bone, const Type type)
{
	writer.beginChunk(0x1002);

	writer.beginChunk(0x1003);
	writer.beginMiniChunk(0x04); writer.writeString(bone.name);
	writer.beginMiniChunk(0x05); writer.writeInteger(bone.index);
	writer.beginMiniChunk(0x0a); writer.writeInteger(bone.unknown0a);
	writer.beginMiniChunk(0x06); writer.write(&bone.translationOffset, sizeof(Vector3));
	writer.beginMiniChunk(0x07); writer.write(&bone.translationScale,  sizeof(Vector3));
	writer.beginMiniChunk(0x08); writer.write(&bone.unknown08, sizeof(Vector3));
	writer.beginMiniChunk(0x09); writer.write(&bone.unknown09, sizeof(Vector3));
	
	if (type == ANIM_FOC)
	{
		writer.beginMiniChunk(0x0E); writer.writeShort(bone.translationIndex);
		writer.beginMiniChunk(0x0F); writer.writeShort(-1);
		writer.beginMiniChunk(0x10); writer.writeShort(bone.rotationIndex);
		writer.beginMiniChunk(0x11); writer.write(&bone.defaultRotation, sizeof(PackedQuaternion));
	}
	writer.endChunk();

	if (type == ANIM_EAW)
	{
		if (bone.translations.size() > 0)
		{
			// Write bone translation data
			writer.beginChunk(0x1004);
			for (unsigned long i = 0; i < m_numFrames; i++)
			{
				writer.write(&bone.translations[i], sizeof(PackedVector3));
			}
			writer.endChunk();
		}

		// Write bone rotation data
		writer.beginChunk(0x1006);
		if (bone.rotations.size() == 0)
		{
			writer.write(&bone.defaultRotation, sizeof(PackedQuaternion));
		}
		else
		{
			for (unsigned long i = 0; i < m_numFrames; i++)
			{
				writer.write(&bone.rotations[i], sizeof(PackedQuaternion));
			}
		}
		writer.endChunk();
	}

	if (bone.visibility.size() > 0)
	{
		// Write visibility data
		writer.beginChunk(0x1007);
		writer.write(&bone.visibility[0], (long)bone.visibility.size());
		writer.endChunk();
	}

	if (bone.unknown1008.size() > 0)
	{
		writer.beginChunk(0x1008);
		writer.write(&bone.unknown1008[0], (long)bone.unknown1008.size());
		writer.endChunk();
	}

	writer.endChunk();
}

void Animation::write(HANDLE file, Type type)
{
	ChunkWriter writer(file);

	unsigned short rotateBlockSize = 0;
	unsigned short transBlockSize  = 0;
	if (type == ANIM_FOC)
	{
		for (vector<BoneAnimation>::iterator p = m_bones.begin(); p != m_bones.end(); p++)
		{
			p->translationIndex = -1;
			if (p->translations.size() > 0)
			{
				p->translationIndex = transBlockSize;
				transBlockSize     += sizeof(PackedVector3) / sizeof(uint16_t);
			}

			p->rotationIndex = -1;
			if (p->rotations.size() > 0)
			{
				p->rotationIndex = rotateBlockSize;
				rotateBlockSize += sizeof(PackedQuaternion) / sizeof(uint16_t);
			}
		}
	}

	writer.beginChunk(0x1000);

	// Write the basic information
	writer.beginChunk(0x1001);
	writer.beginMiniChunk(0x01); writer.writeInteger(m_numFrames);
	writer.beginMiniChunk(0x02); writer.writeFloat(m_fps);
	writer.beginMiniChunk(0x03); writer.writeInteger((unsigned long)m_bones.size());
	if (type == ANIM_FOC)
	{
		writer.beginMiniChunk(0x0B); writer.writeInteger(rotateBlockSize);
		writer.beginMiniChunk(0x0C); writer.writeInteger(transBlockSize);
		writer.beginMiniChunk(0x0D); writer.writeInteger(0);
	}
	writer.endChunk();

	// Write the bones
	for (vector<BoneAnimation>::const_iterator p = m_bones.begin(); p != m_bones.end(); p++)
	{
		writeBoneAnimation(writer, *p, type);
	}

	// For Forces of Corruption, write the translation and rotation data
	if (type == ANIM_FOC)
	{
		if (transBlockSize > 0)
		{
			// Write translation data
			writer.beginChunk(0x100a);
			for (unsigned long f = 0; f < m_numFrames; f++)
			for (vector<BoneAnimation>::const_iterator p = m_bones.begin(); p != m_bones.end(); p++)
			{
				if (p->translationIndex != -1)
				{
					writer.write(&p->translations[f], sizeof(PackedVector3));
				}
			}
			writer.endChunk();
		}

		if (rotateBlockSize > 0)
		{
			// Write rotation data
			writer.beginChunk(0x1009);
			for (unsigned long f = 0; f < m_numFrames; f++)
			for (vector<BoneAnimation>::const_iterator p = m_bones.begin(); p != m_bones.end(); p++)
			{
				if (p->rotationIndex != -1)
				{
					writer.write(&p->rotations[f], sizeof(PackedQuaternion));
				}
			}
			writer.endChunk();
		}
	}

	writer.endChunk();
}

void Animation::readBoneAnimation(ChunkReader& reader, BoneAnimation& bone)
{
	ChunkType type;

	Verify(reader.next() == 0x1003);
	Verify(reader.nextMini() == 0x04); bone.name  = reader.readString();
	Verify(reader.nextMini() == 0x05); bone.index = reader.readInteger();
	
	type = reader.nextMini();
	bone.unknown0a = 0;
	if (type == 0x0a)
	{
		bone.unknown0a = reader.readInteger();
		type = reader.nextMini();
	}
	Verify(type              == 0x06); reader.read(&bone.translationOffset, sizeof(Vector3));
	Verify(reader.nextMini() == 0x07); reader.read(&bone.translationScale,  sizeof(Vector3));
	Verify(reader.nextMini() == 0x08); reader.read(&bone.unknown08, sizeof(Vector3));
	Verify(reader.nextMini() == 0x09); reader.read(&bone.unknown09, sizeof(Vector3));
	
	if (getType() == ANIM_FOC)
	{
		Verify(reader.nextMini() == 0x0E); bone.translationIndex = (int16_t)reader.readShort();
		Verify(reader.nextMini() == 0x0F); Verify((int16_t)reader.readShort() == -1);
		Verify(reader.nextMini() == 0x10); bone.rotationIndex = (int16_t)reader.readShort();
		Verify(reader.nextMini() == 0x11); reader.read(&bone.defaultRotation, sizeof(PackedQuaternion));
	}
	Verify(reader.nextMini() == -1);

	if (getType() == ANIM_EAW)
	{
		type = reader.next();
		if (type == 0x1004)
		{
			// Read bone translation data
			Verify(reader.size() == m_numFrames * sizeof(PackedVector3));
			bone.translations.resize(m_numFrames);
			for (unsigned long i = 0; i < m_numFrames; i++)
			{
				reader.read(&bone.translations[i], sizeof(PackedVector3));
			}
			type = reader.next();
		}

		// Read bone rotation data
		Verify(type == 0x1006);

		if (reader.size() == sizeof(PackedQuaternion))
		{
			reader.read(&bone.defaultRotation, sizeof(PackedQuaternion));
		}
		else
		{
			Verify(reader.size() == m_numFrames * sizeof(PackedQuaternion));
			bone.rotations.resize(m_numFrames);
			for (unsigned long i = 0; i < m_numFrames; i++)
			{
				reader.read(&bone.rotations[i], sizeof(PackedQuaternion));
			}
			bone.defaultRotation = bone.rotations[0];
		}
	}

	type = reader.next();
	if (type == 0x1007)
	{
		// Read visibility data
		Verify(reader.size() == (m_numFrames + 7) / 8);
		bone.visibility.resize(reader.size());
		reader.read(&bone.visibility[0], reader.size());
		type = reader.next();
	}

	if (type == 0x1008)
	{
		bone.unknown1008.resize(reader.size());
		reader.read(&bone.unknown1008[0], reader.size());
		type = reader.next();
	}

	Verify(type == -1);
}

Animation::Animation(HANDLE file)
{
	ChunkReader reader(file);
	ChunkType   type;

	m_type = ANIM_EAW;

	unsigned long rotateBlockSize = 0;
	unsigned long transBlockSize  = 0;

	Verify(reader.next() == 0x1000);

	// Read the basic information
	Verify(reader.next() == 0x1001);
	Verify(reader.nextMini() == 0x01); m_numFrames = reader.readInteger();
	Verify(reader.nextMini() == 0x02); m_fps       = reader.readFloat();
	Verify(reader.nextMini() == 0x03); m_bones.resize(reader.readInteger());
	type = reader.nextMini();
	if (type != -1)
	{
		m_type = ANIM_FOC;
		Verify(type == 0x0B);			   rotateBlockSize = reader.readInteger();
		Verify(reader.nextMini() == 0x0C); transBlockSize  = reader.readInteger();
		Verify(reader.nextMini() == 0x0D); Verify(reader.readInteger() == 0);
		type = reader.nextMini();
	}
	Verify(type == -1);

	// Read the bones
	for (vector<BoneAnimation>::iterator p = m_bones.begin(); p != m_bones.end(); p++)
	{
		Verify(reader.next() == 0x1002);
		readBoneAnimation(reader, *p);
	}

	// For Forces of Corruption, read the translation and rotation data
	if (getType() == ANIM_FOC)
	{
		vector<uint16_t> transData;
		vector<uint16_t> rotateData;

		if (transBlockSize > 0)
		{
			// Read translation data
			Verify(reader.next() == 0x100a);
			Verify(reader.size() == transBlockSize * m_numFrames * sizeof(uint16_t));
			transData.resize(transBlockSize * m_numFrames);
			reader.read(&transData[0], reader.size());
		}

		if (rotateBlockSize > 0)
		{
			// Read rotation data
			Verify(reader.next() == 0x1009);
			Verify(reader.size() == rotateBlockSize * m_numFrames * sizeof(uint16_t));
			rotateData.resize(rotateBlockSize * m_numFrames);
			reader.read(&rotateData[0], reader.size());
		}

		for (vector<BoneAnimation>::iterator p = m_bones.begin(); p != m_bones.end(); p++)
		{
			BoneAnimation& bone = *p;
			if (bone.rotationIndex != -1)
			{
				bone.rotations.resize(m_numFrames);
				for (unsigned long f = 0; f < m_numFrames; f++)
				{
					bone.rotations[f] = *(PackedQuaternion*)&rotateData[f * rotateBlockSize + bone.rotationIndex];
				}
			}

			if (bone.translationIndex != -1)
			{
				bone.translations.resize(m_numFrames);
				for (unsigned long f = 0; f < m_numFrames; f++)
				{
					bone.translations[f] = *(PackedVector3*)&transData[f * transBlockSize + bone.translationIndex];
				}
			}
		}
	}

	Verify(reader.next() == -1);
	Verify(reader.next() == -1);
}
