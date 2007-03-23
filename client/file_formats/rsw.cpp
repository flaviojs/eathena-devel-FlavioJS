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
** Special thanks to:
**    Borf for quadtree info
**    ximosoft and who he's thanking for info -> http://rolaboratory.ximosoft.com/file-format/rsw/
**    Gravity
**    more will surelly come when I check more info about this format ;D
**
** $Id$
*/
#include "fx.h"
#include <stdio.h>

struct Vector3D
{
	FXfloat x;// maybe
	FXfloat y;// maybe
	FXfloat z;// maybe

	void set(FXfloat x, FXfloat y, FXfloat z)	{ this->x = x; this->y = y; this->z = z; }
};

struct RSW
{
	FXchar magic[4];// "GRSW"
	FXchar major_ver;// version <= 2.1
	FXchar minor_ver;
	FXchar ini_file[40];// ini file (in alpha only?) - ignored
	FXchar gnd_file[40];// ground file
	FXchar gat_file[40];// altitude file (version >= 1.4)
	FXchar src_file[40];// source file (in alpha only?) // scr_file???
	FXfloat water_height;// water height (version >= 1.3)
	FXint water_type;// water type (version >= 1.8)
	FXfloat water_amplitude;// water amplitude (version >= 1.8)
	FXfloat water_phase;// water phase (version >= 1.8)
	FXfloat surface_curve_level;// water surface curve level (version >= 1.8)
	FXint texture_cycling;// texture cycling (version >= 1.9)
	FXint unk1;// (version >= 1.5)
	FXint unk2;// (version >= 1.5)
	Vector3D vec1;// (version >= 1.5)
	Vector3D vec2;// (version >= 1.5)
	FXfloat unk3;// (version >= 1.7)
	FXint unk4;// (version >= 1.6)
	FXint unk5;// (version >= 1.6)
	FXint unk6;// (version >= 1.6)
	FXint unk7;// (version >= 1.6)
	FXint object_count;

	// TODO add linked list(s) of objects

	// TODO add quadtree of "scene graph nodes"

	bool IsCompatibleWith(FXchar major, FXchar minor)	{ return ( ( major_ver == major && minor_ver >= minor ) || major_ver > major ); }
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

			// src file - source file
			pos = f.position();
			f.readBlock(d.src_file, 40);
			printf("%lld:src_file=%.40s\n", pos, d.src_file);
			
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
