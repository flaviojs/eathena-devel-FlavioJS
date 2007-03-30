/*
** 2003 March 25
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This file uses the fox toolkit library.
**
** Special thanks to:
**    Borf for quadtree info
**    ximosoft and who he's thanking for info -> http://rolaboratory.ximosoft.com/file-format/rsw/
**    Gravity
**    more will surely come when I check more info about this format ;D
**
** $Id$
*/
#ifndef _RSW_H_
#define _RSW_H_

#include "fx.h"
#include "FXVec2f.h"
#include "FXVec3f.h"
#include "FXArray.h"
#include <vector>

//-----------------------------------------------------------------------------
namespace NRSW {
//-----------------------------------------------------------------------------
// 1.2 - base supported version
// 1.3 - water height added, object type 1 has more data
// 1.4 - gat filename added
// 1.5 - 2 ints and 2 Vector3D's (unk1/unk2/vec1/vec2) added
// 1.6 - 4 unknown ints(?) (unk4/unk5/unk6/unk7) added
// 1.7 - unknown float (unk3) added
// 1.8 - water type, water amplitude, water phase, surface curve Level added
// 1.9 - texture cycling added
// 2.0 - float added to object type 3 (sound object)
// 2.1 - quadtree of scene areas added (6 levels deep)

/// Type of object
/// TODO what kind of objects exist in older versions
enum ObjectType
{
	UnknownObjectType = 0,
	ModelObjectType = 1,
	LightObjectType = 2,
	SoundObjectType = 3,
	EffectObjectType = 4
};

struct RSWObject
{
	FXlong position;
	ObjectType object_type;

	RSWObject() : object_type(UnknownObjectType) , position(0){}
	RSWObject(FXlong position) : object_type(UnknownObjectType) , position(position){}
	RSWObject(ObjectType object_type) : object_type(object_type) , position(0){}
	RSWObject(ObjectType object_type, FXlong position) : object_type(object_type) , position(position){}
};

/// Model object - 248 bytes
struct ModelObject : public RSWObject
{
	FXchar name[40];// (version >= 1.3)
	FXint unk1;// (version >= 1.3)
	FXfloat unk2;// (version >= 1.3)
	FXfloat unk3;// (version >= 1.3)
	FXchar filename[40];
	FXchar reserved[40];
	FXchar type[20];
	FXchar sound[20];
	FXchar todo1[40];
	FXfloat pos_x;
	FXfloat pos_y;
	FXfloat pos_z;
	FXfloat rot_x;
	FXfloat rot_y;
	FXfloat rot_z;
	FXfloat scale_x;
	FXfloat scale_y;
	FXfloat scale_z;

	enum Version {
		VersionBase,
		Version0103// rsw version >= 1.3
	};

	void load(FXStream& store, Version v = VersionBase);
	void save(FXStream& store, Version v = VersionBase) const;

	ModelObject():RSWObject(ModelObjectType){}
	ModelObject(FXlong position):RSWObject(ModelObjectType,position){}
};

/// Light object - 108 bytes
struct LightObject : public RSWObject
{
	FXchar name[40];
	FXfloat pos_x;
	FXfloat pos_y;
	FXfloat pos_z;
	FXchar todo1[40];
	FXfloat color_r;
	FXfloat color_g;
	FXfloat color_b;
	FXfloat todo2;

	void load(FXStream& store);
	void save(FXStream& store) const;

	LightObject():RSWObject(LightObjectType){}
	LightObject(FXlong position):RSWObject(LightObjectType,position){}
};

/// Sound object - 192 bytes
struct SoundObject : public RSWObject
{
	FXchar name[80];
	FXchar filename[80];
	FXfloat todo1;
	FXfloat todo2;
	FXfloat todo3;
	FXfloat todo4;
	FXfloat todo5;
	FXfloat todo6;
	FXfloat todo7;
	FXfloat todo8;// (version >= 2.0)

	enum Version {
		VersionBase,
		Version0200// rsw version >= 2.0
	};

	void load(FXStream& store, Version v = VersionBase);
	void save(FXStream& store, Version v = VersionBase) const;

	SoundObject():RSWObject(SoundObjectType){}
	SoundObject(FXlong position):RSWObject(SoundObjectType,position){}
};

/// Effect object 116 bytes
struct EffectObject : public RSWObject
{
	FXchar name[40];
	FXfloat todo1;
	FXfloat todo2;
	FXfloat todo3;
	FXfloat todo4;
	FXfloat todo5;
	FXfloat todo6;
	FXfloat todo7;
	FXfloat todo8;
	FXfloat todo9;
	FXint category;
	FXfloat pos_x;
	FXfloat pos_y;
	FXfloat pos_z;
	FXint type;
	FXfloat loop;
	FXfloat todo10;
	FXfloat todo11;
	FXint todo12;
	FXint todo13;

	void load(FXStream& store);
	void save(FXStream& store) const;

	EffectObject():RSWObject(EffectObjectType){}
	EffectObject(FXlong position):RSWObject(EffectObjectType,position){}
};

/// Resource World
struct RSW
{
	FXchar magic[4];// "GRSW"
	FXchar major_ver;// version <= 2.1
	FXchar minor_ver;
	FXchar ini_file[40];// ini file (in alpha only?) - ignored
	FXchar gnd_file[40];// ground file
	FXchar gat_file[40];// altitude file (version >= 1.4)
	FXchar scr_file[40];// (in alpha only?)
	FXfloat water_height;// water height (version >= 1.3)
	FXint water_type;// water type (version >= 1.8)
	FXfloat water_amplitude;// water amplitude (version >= 1.8)
	FXfloat water_phase;// water phase (version >= 1.8)
	FXfloat surface_curve_level;// water surface curve level (version >= 1.8)
	FXint texture_cycling;// texture cycling (version >= 1.9)
	FXint unk1;// angle(?) in degrees (version >= 1.5)
	FXint unk2;// angle(?) in degrees (version >= 1.5)
	FXVec3f vec1;// (version >= 1.5)
	FXVec3f vec2;// (version >= 1.5)
	FXfloat unk3;// ignored (version >= 1.7)
	FXint unk4;// (version >= 1.6)
	FXint unk5;// (version >= 1.6)
	FXint unk6;// (version >= 1.6)
	FXint unk7;// (version >= 1.6)
	FXint object_count;

	// objects
	RSWObject** objects;
	std::vector< ModelObject* > models;
	std::vector< LightObject* > lights;
	std::vector< SoundObject* > sounds;
	std::vector< EffectObject* > effects;

	/// If this RSW is compatible with the target version (version >= target)
	bool IsCompatibleWith(FXchar major_ver, FXchar minor_ver) const;

	void load(FXStream& store);
	void save(FXStream& store) const;

	friend FXStream& operator>>(FXStream& store,RSW& rsw);
	friend FXStream& operator<<(FXStream& store,const RSW& rsw);
};

//-----------------------------------------------------------------------------
}// namespace NRSW
//-----------------------------------------------------------------------------

#endif // _RSW_H_