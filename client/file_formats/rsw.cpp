/*
** 2003 March 24
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
#include "rsw.h"

using namespace NRSW;
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

//-----------------------------------------------------------------------------
// RSW

/// If this RSW is compatible with the target version (version >= target)
bool RSW::IsCompatibleWith(FXchar major_ver, FXchar minor_ver) const
{
	return ( ( this->major_ver == major_ver && this->minor_ver >= minor_ver ) || this->major_ver > major_ver );
}

/// Load RSW from a stream
FXStream& operator>>(FXStream& store, RSW& rsw)
{
	rsw.load(store);
	return store;
}
void RSW::load(FX::FXStream &store)
{
	bool bigEndian = store.isBigEndian();
	store.setBigEndian(false);// data is in little endian
	store.load(magic, 4);
	store >> major_ver;
	store >> minor_ver;
	store.load(ini_file, 40);
	store.load(gnd_file, 40);
	if( IsCompatibleWith(1,4) )
		store.load(gat_file, 40);
	else
		memset(gat_file, 0, 40);
	if( IsCompatibleWith(1,3) )
		store >> water_height;
	else
		water_height = 0.0f;
	if( IsCompatibleWith(1,8) )
	{
		store >> water_type;
		store >> water_amplitude;
		store >> water_phase;
		store >> surface_curve_level;
	}
	else
	{
		water_type = 0;
		water_amplitude = 1.0;
		water_phase = 2.0;
		surface_curve_level = 0.5;
	}
	if( IsCompatibleWith(1,9) )
		store >> texture_cycling;
	else
		texture_cycling = 3;
	if( IsCompatibleWith(1,4) )
	{
		store >> unk1;
		store >> unk2;
		store >> vec1;
		store >> vec2;
	}
	else
	{
		unk1 = 45;
		unk2 = 45;
		FXfloat v1[3] = { 1.0f , 1.0f , 1.0f };
		vec1 = v1;
		FXfloat v2[3] = { 0.3f , 0.3f , 0.3f };
		vec2 = v2;
	}
	if( IsCompatibleWith(1,7) )
		store >> unk3;// ignored
	if( IsCompatibleWith(1,6) )
	{
		store >> unk4;
		store >> unk5;
		store >> unk6;
		store >> unk7;
	}
	else
	{
		unk4 = -500;
		unk5 = 500;
		unk6 = -500;
		unk7 = 500;
	}
	store >> object_count;
	// objects
	fxcalloc((void**)&objects, object_count*sizeof(RSWObject*));
	models.clear();
	lights.clear();
	sounds.clear();
	effects.clear();
	for( FXint i = 0; i < object_count; ++i )
	{
		FXint type;
		store >> type;
		switch( type )
		{
		case ModelObjectType:
			{
				ModelObject* model = new ModelObject(store.position());
				if( IsCompatibleWith(1,3) )
					model->load(store, ModelObject::Version0103);
				else
					model->load(store, ModelObject::VersionBase);
				models.push_back(model);
				objects[i] = model;
				break;
			}
		case LightObjectType:
			{
				LightObject* light = new LightObject(store.position());
				light->load(store);
				lights.push_back(light);
				objects[i] = light;
				break;
			}
		case SoundObjectType:
			{
				SoundObject* sound = new SoundObject(store.position());
				if( IsCompatibleWith(2,0) )
					sound->load(store, SoundObject::Version0200);
				else
					sound->load(store, SoundObject::VersionBase);
				sounds.push_back(sound);
				objects[i] = sound;
				break;
			}
		case EffectObjectType:
			{
				EffectObject* effect = new EffectObject(store.position());
				effect->load(store);
				effects.push_back(effect);
				objects[i] = effect;
				break;
			}
		default:
			{
				fxerror("unknown type %d for RSW object %d/%d at offset %lld\n", type, (i+1), object_count, store.position());
				objects[i] = new RSWObject(store.position());
			}
		}
	}
	// TODO quadtree
	store.setBigEndian(bigEndian);// revert to the previous endianess
}

/// Save RSW to a stream
FXStream& operator<<(FXStream& store, const RSW& rsw)
{
	rsw.save(store);
	return store;
}
void RSW::save(FX::FXStream &store) const
{
	bool bigEndian = store.isBigEndian();
	store.save(magic, 4);
	store << major_ver;
	store << minor_ver;
	store.save(ini_file, 40);
	store.save(gnd_file, 40);
	if( IsCompatibleWith(1,4) )
		store.save(gat_file, 40);
	if( IsCompatibleWith(1,3) )
		store << water_height;
	if( IsCompatibleWith(1,8) )
	{
		store << water_type;
		store << water_amplitude;
		store << water_phase;
		store << surface_curve_level;
	}
	if( IsCompatibleWith(1,9) )
		store << texture_cycling;
	if( IsCompatibleWith(1,4) )
	{
		store << unk1;
		store << unk2;
		store << vec1;
		store << vec2;
	}
	if( IsCompatibleWith(1,7) )
		store << unk3;// ignored
	if( IsCompatibleWith(1,6) )
	{
		store << unk4;
		store << unk5;
		store << unk6;
		store << unk7;
	}
	store << object_count;
	// TODO objects
	// TODO quadtree
	store.setBigEndian(bigEndian);
}

//-----------------------------------------------------------------------------
// Model object

void ModelObject::load(FXStream& store, ModelObject::Version v)
{
	if( v == Version0103 )
	{
		store.load(name, 40);
		store >> unk1;
		store >> unk2;
		store >> unk3;
	}
	else
	{
		name[0] = 0;
		unk1 = 0;
		unk2 = 1.0f;
		unk3 = 0;
	}
	store.load(filename, 40);
	store.load(reserved, 40);
	store.load(type, 20);
	store.load(sound, 20);
	store.load(todo1, 40);
	store >> pos_x;
	store >> pos_y;
	store >> pos_z;
	store >> rot_x;
	store >> rot_y;
	store >> rot_z;
	store >> scale_x;
	store >> scale_y;
	store >> scale_z;
}

void ModelObject::save(FXStream& store, ModelObject::Version v) const
{
	if( v == Version0103 )
	{
		store.save(name, 40);
		store << unk1;
		store << unk2;
		store << unk3;
	}
	store.save(filename, 40);
	store.save(reserved, 40);
	store.save(type, 20);
	store.save(sound, 20);
	store.save(todo1, 40);
	store << pos_x;
	store << pos_y;
	store << pos_z;
	store << rot_x;
	store << rot_y;
	store << rot_z;
	store << scale_x;
	store << scale_y;
	store << scale_z;
}

//-----------------------------------------------------------------------------
// Light object

void LightObject::load(FXStream& store)
{
	store.load(name, 40);
	store >> pos_x;
	store >> pos_y;
	store >> pos_z;
	store.load(todo1, 40);
	store >> color_r;
	store >> color_g;
	store >> color_b;
	store >> todo2;
}

void LightObject::save(FXStream& store) const
{
	store.save(name, 40);
	store << pos_x;
	store << pos_y;
	store << pos_z;
	store.save(todo1, 40);
	store << color_r;
	store << color_g;
	store << color_b;
	store << todo2;
}

//-----------------------------------------------------------------------------
// Sound object

void SoundObject::load(FXStream& store, Version v)
{
	store.load(name, 80);
	store.load(filename, 80);
	store >> todo1;
	store >> todo2;
	store >> todo3;
	store >> todo4;
	store >> todo5;
	store >> todo6;
	store >> todo7;
	if( v == Version0200 )
		store >> todo8;
	else
		todo8 = 4.0f;
}

void SoundObject::save(FXStream& store, Version v) const
{
	store.save(name, 80);
	store.save(filename, 80);
	store << todo1;
	store << todo2;
	store << todo3;
	store << todo4;
	store << todo5;
	store << todo6;
	store << todo7;
	if( v == Version0200 )
		store << todo8;
}

//-----------------------------------------------------------------------------
// Effect object

void EffectObject::load(FXStream& store)
{
	position = store.position();

	store.load(name, 40);
	store >> todo1;
	store >> todo2;
	store >> todo3;
	store >> todo4;
	store >> todo5;
	store >> todo6;
	store >> todo7;
	store >> todo8;
	store >> todo9;
	store >> category;
	store >> pos_x;
	store >> pos_y;
	store >> pos_z;
	store >> type;
	store >> loop;
	store >> todo10;
	store >> todo11;
	store >> todo12;
	store >> todo13;
}

void EffectObject::save(FXStream& store) const
{
	store.save(name, 40);
	store << todo1;
	store << todo2;
	store << todo3;
	store << todo4;
	store << todo5;
	store << todo6;
	store << todo7;
	store << todo8;
	store << todo9;
	store << category;
	store << pos_x;
	store << pos_y;
	store << pos_z;
	store << type;
	store << loop;
	store << todo10;
	store << todo11;
	store << todo12;
	store << todo13;
}

//-----------------------------------------------------------------------------
// old

//#define OLD_DEBUG_RSW
#ifdef OLD_DEBUG_RSW
#include <stdio.h>

struct Vector3D
{
	FXfloat x;// maybe
	FXfloat y;// maybe
	FXfloat z;// maybe

	void set(FXfloat x, FXfloat y, FXfloat z)	{ this->x = x; this->y = y; this->z = z; }
};

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
int main(int argc, char argv[])
{
	FXStream s;
	FXString* filelist;
	FXint files = FXDir::listFiles(filelist, ".", "*.rsw", FXDir::NoDirs);
	for( FXint filenum = 0; filenum < files; ++filenum )
	{
		printf("%d/%d %s\n", (filenum+1), files, filelist[filenum].text());
		FXFile f(filelist[filenum],FXIO::ReadOnly);
		if( f.isOpen() )
		{
			RSW d;
			FXlong pos;

			// magic
			pos = f.position();
			f.readBlock(d.magic, 4);
			printf("%lld:magic=%.4s\n", pos, d.magic);

			// major.minor version
			pos = f.position();
			f.readBlock(&d.major_ver, 1);
			f.readBlock(&d.minor_ver, 1);
			printf("%lld:version=%d.%d\n", pos, d.major_ver, d.minor_ver);

			// ini file (in alpha only?) - ignored
			pos = f.position();
			f.readBlock(d.ini_file, 40);
			printf("%lld:ini_file=%.40s\n", pos, d.ini_file);

			// gnd file - ground file
			pos = f.position();
			f.readBlock(d.gnd_file, 40);
			printf("%lld:gnd_file=%.40s\n", pos, d.gnd_file);

			// gat file - altitude file
			if( d.IsCompatibleWith(1,4) )
			{
				pos = f.position();
				f.readBlock(d.gat_file, 40);
				printf("%lld:gat_file=%.40s\n", pos, d.gat_file);
			}
			else
				d.gat_file[0] = 0;

			// scr file - ???
			pos = f.position();
			f.readBlock(d.scr_file, 40);
			printf("%lld:scr_file=%.40s\n", pos, d.scr_file);
			
			// water height
			if( d.IsCompatibleWith(1,3) )
			{
				pos = f.position();
				f.readBlock(&d.water_height, 4);
				printf("%lld:water_height=%f\n", pos, d.water_height);
			}
			else
				d.water_height = 0.0;

			// water type
			// water amplitude 
			// water phase
			// surface curve Level
			if( d.IsCompatibleWith(1,8) )
			{
				pos = f.position();
				f.readBlock(&d.water_type, 4);
				printf("%lld:water_type=%d\n", pos, d.water_type);

				pos = f.position();
				f.readBlock(&d.water_amplitude, 4);
				printf("%lld:water_amplitude=%f\n", pos, d.water_amplitude);

				pos = f.position();
				f.readBlock(&d.water_phase, 4);
				printf("%lld:water_phase=%f\n", pos, d.water_phase);

				pos = f.position();
				f.readBlock(&d.surface_curve_level, 4);
				printf("%lld:surface_curve_level=%f\n", pos, d.surface_curve_level);
			}
			else
			{
				d.water_type = 0;
				d.water_amplitude = 1.0;
				d.water_phase = 2.0;
				d.surface_curve_level = 0.5;
			}

			// texture cycling
			if( d.IsCompatibleWith(1,9) )
			{
				pos = f.position();
				f.readBlock(&d.texture_cycling, 4);
				printf("%lld:texture_cycling=%d\n", pos, d.texture_cycling);
			}
			else
				d.texture_cycling = 3;

			// int
			// int
			// vector 3D
			// vector 3D
			if( d.IsCompatibleWith(1,5) )
			{
				pos = f.position();
				f.readBlock(&d.unk1, 4);
				printf("%lld:unk1=%d\n", pos, d.unk1);

				pos = f.position();
				f.readBlock(&d.unk2, 4);
				printf("%lld:unk2=%d\n", pos, d.unk2);

				pos = f.position();
				f.readBlock(&d.vec1, 12);
				printf("%lld:vec1={%f,%f,%f}\n", pos, d.vec1.x, d.vec1.y, d.vec1.z);

				pos = f.position();
				f.readBlock(&d.vec2, 12);
				printf("%lld:vec2={%f,%f,%f}\n", pos, d.vec2.x, d.vec2.y, d.vec2.z);
			}
			else
			{
				d.unk1 = 45;
				d.unk2 = 45;
				d.vec1.set(1.0f, 1.0f, 1.0f);
				d.vec2.set(0.3f, 0.3f, 0.3f);
			}

			// unknown calculation involving unk1/unk2/vec1/vec2 to calculate a Vector3D(maybe)
			// TODO

			// float
			if( d.IsCompatibleWith(1,7) )
			{
				pos = f.position();
				f.readBlock(&d.unk3, 4);
				printf("%lld:unk3=%f\n", pos, d.unk3);// ignored
			}

			// int
			// int
			// int
			// int
			if( d.IsCompatibleWith(1,6) )
			{
				pos = f.position();
				f.readBlock(&d.unk4, 4);
				printf("%lld:unk4=%d\n", pos, d.unk4);

				pos = f.position();
				f.readBlock(&d.unk5, 4);
				printf("%lld:unk5=%d\n", pos, d.unk5);

				pos = f.position();
				f.readBlock(&d.unk6, 4);
				printf("%lld:unk6=%d\n", pos, d.unk6);

				pos = f.position();
				f.readBlock(&d.unk7, 4);
				printf("%lld:unk7=%d\n", pos, d.unk7);
			}
			else
			{
				d.unk4 = -500;
				d.unk5 = 500;
				d.unk6 = -500;
				d.unk7 = 500;
			}

			// object count
			pos = f.position();
			f.readBlock(&d.object_count, 4);
			printf("%lld:object_count=%d\n", pos, d.object_count);

			for( FXint objnum = 0; objnum < d.object_count; ++objnum )
			{
				pos = f.position();
				printf("%lld:%d/%d\n", pos, (objnum + 1), d.object_count);
				FXlong subpos;

				// object type
				subpos = f.position() - pos;
				FXint obj_type;
				f.readBlock(&obj_type, 4);
				//printf("%lld:%lld:obj_type=%d\n", pos, subpos, obj_type);

				switch( obj_type )
				{
				case 1:// sizeof = ( version >= 1.3 ? 196 : 248 ) -> model object
					{
						// 40 byte string
						// TODO 4 bytes
						// float
						// TODO 4 bytes
						// TODO 196 bytes of data
						FXchar obj1_unk1[40];
						FXint obj1_unk2;
						FXfloat obj1_unk3;
						FXint obj1_unk4;
						if( d.IsCompatibleWith(1,3) )
						{
							// ???
							subpos = f.position() - pos;
							f.readBlock(obj1_unk1, 40);
							//printf("%lld:%lld:obj1_unk1=%.40s\n", pos, subpos, obj1_unk1);

							// ???
							subpos = f.position() - pos;
							f.readBlock(&obj1_unk2, 4);
							//printf("%lld:%lld:obj1_unk2=%d\n", pos, subpos, obj1_unk2);

							// ???
							subpos = f.position() - pos;
							f.readBlock(&obj1_unk3, 4);
							//printf("%lld:%lld:obj1_unk3=%f\n", pos, subpos, obj1_unk3);

							// ???
							subpos = f.position() - pos;
							f.readBlock(&obj1_unk4, 4);
							//printf("%lld:%lld:obj1_unk4=%d\n", pos, subpos, obj1_unk4);
						}
						else
						{
							obj1_unk1[0] = 0;
							obj1_unk2 = 0;
							obj1_unk3 = 1.0f;
							obj1_unk4 = 0;
						}

						// ???
						subpos = f.position() - pos;
						FXchar obj1_unk5[196];
						f.readBlock(obj1_unk5, 196);
						//printf("%lld:%lld:obj1_unk5(196 bytes)\n", pos, subpos);
					}
					break;
				case 2:// sizeof = 108 -> light object
					{
						// TODO 108 bytes of data
						subpos = f.position() - pos;
						FXchar obj2_unk1[108];
						f.readBlock(obj2_unk1, 108);
						//printf("%lld:%lld:obj2_unk1(108 bytes)\n", pos, subpos);
					}
					break;
				case 3:// sizeof = ( version >= 2.0 ? 192 : 188 ) -> sound object
					{
						// 80 byte string (nul terminated)
						// 80 byte string (nul terminated)
						// TODO 7*4 bytes of data
						// float
						FXchar obj3[192];
						if( d.IsCompatibleWith(2,0) )
						{
							// ???
							subpos = f.position() - pos;
							f.readBlock(obj3, 192);
							//printf("%lld:%lld:obj3(192 bytes)\n", pos, subpos);
						}
						else
						{
							// ???
							subpos = f.position() - pos;
							f.readBlock(obj3, 188);
							//printf("%lld:%lld:obj3(188 bytes)\n", pos, subpos);

							FXfloat obj3_float188 = 4.0;
							memcpy(obj3+188, &obj3_float188, 4);
						}
					}
					break;
				case 4:// sizeof = 116 -> effect object
					{
						// TODO 116 bytes of data
						subpos = f.position() - pos;
						FXchar obj4[116];
						f.readBlock(obj4, 116);
						//printf("%lld:%lld:obj4(116 bytes)\n", pos, subpos);
					}
					break;
				}
			}

			if( d.IsCompatibleWith(2,1) )
			{
				pos = f.position();
				void ParseSceneGraphNode(FXFile& f, FXint level, FXint index);
				ParseSceneGraphNode(f, 0, 0);
				printf("%lld:\"SceneGraphNode Load.\"\n", pos);
				// debug message "SceneGraphNode Load."
			}
			// bytes remaining
			pos = f.position();
			FXlong size = f.size();
			printf("%lld:file_size=%lld -> %lld extra bytes\n", pos, size, (size - pos));
		}
		printf("\n");
	}
	return 0;
}

void ParseSceneGraphNode(FXFile& f, FXint level, FXint index)
{
	FXlong pos = f.position();
	//printf("%lld: Schene Graph Node - level %d, index %d\n", pos, level, index);
	FXlong subpos;

	// side1?
	subpos = f.position() - pos;
	Vector3D side1;
	f.readBlock(&side1, 12);// maybe not a Vector3D, read individualy
	//printf("%lld:%lld:side1={%f,%f,%f}\n", pos, subpos, side1.x, side1.y, side1.z);

	// side2?
	subpos = f.position() - pos;
	Vector3D side2;
	f.readBlock(&side2, 12);// maybe not a Vector3D, read individualy
	//printf("%lld:%lld:side2={%f,%f,%f}\n", pos, subpos, side2.x, side2.y, side2.z);

	// side3?
	subpos = f.position() - pos;
	Vector3D side3;
	f.readBlock(&side3, 12);// maybe not a Vector3D, read individualy
	//printf("%lld:%lld:side3={%f,%f,%f}\n", pos, subpos, side3.x, side3.y, side3.z);

	// side4?
	subpos = f.position() - pos;
	Vector3D side4;
	f.readBlock(&side4, 12);// maybe not a Vector3D, read individualy
	//printf("%lld:%lld:side4={%f,%f,%f}\n", pos, subpos, side4.x, side4.y, side4.z);

	if( level < 6 )// 6 levels in total
	{
		for( FXint idx = 0; idx < 4; ++idx )
		{
			ParseSceneGraphNode(f, level + 1, idx);
		}
	}
}

#endif// OLD_DEBUG_RSW
