/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "bf.h"

#include "../mem.h"
#include "../archive.h"
#include "../stage.h"
#include "../random.h"
#include "../main.h"

//Boyfriend skull fragments
static SkullFragment char_bf_skull[15] = {
	{ 1 * 8, -87 * 8, -13, -13},
	{ 9 * 8, -88 * 8,   5, -22},
	{18 * 8, -87 * 8,   9, -22},
	{26 * 8, -85 * 8,  13, -13},
	
	{-3 * 8, -82 * 8, -13, -11},
	{ 8 * 8, -85 * 8,  -9, -15},
	{20 * 8, -82 * 8,   9, -15},
	{30 * 8, -79 * 8,  13, -11},
	
	{-1 * 8, -74 * 8, -13, -5},
	{ 8 * 8, -77 * 8,  -9, -9},
	{19 * 8, -75 * 8,   9, -9},
	{26 * 8, -74 * 8,  13, -5},
	
	{ 5 * 8, -73 * 8, -5, -3},
	{14 * 8, -76 * 8,  9, -6},
	{26 * 8, -67 * 8, 15, -3},
};

//Boyfriend player types
enum
{
	BF_ArcMain_BF0,
	BF_ArcMain_BF1,
	BF_ArcMain_BF2,
	BF_ArcMain_BF5,
	BF_ArcMain_BF6,
	BF_ArcMain_Alt0,
	BF_ArcMain_Alt1,
	BF_ArcMain_Dead0, //BREAK
	
	BF_ArcMain_Max,
};

enum
{
	BF_ArcDead_Dead1, //Mic Drop
	BF_ArcDead_Dead2, //Twitch
	BF_ArcDead_Retry, //Retry prompt
	
	BF_ArcDead_Max,
};

#define BF_Arc_Max BF_ArcMain_Max

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main, arc_dead;
	CdlFILE file_dead_arc; //dead.arc file position
	IO_Data arc_ptr[BF_Arc_Max];
	
	Gfx_Tex tex, tex_retry;
	u8 frame, tex_id;
	
	u8 retry_bump;
	
	SkullFragment skull[COUNT_OF(char_bf_skull)];
	u8 skull_scale;
} Char_BF;

//Boyfriend player definitions
static const CharFrame char_bf_frame[] = {
	{BF_ArcMain_BF0, {  0,  54, 69,  73}, { 66,  73}}, //0 idle 1
	{BF_ArcMain_BF0, {128,  51, 68,  76}, { 63,  76}}, //1 idle 2
	{BF_ArcMain_BF0, {  0, 177, 67,  79}, { 60,  79}}, //2 idle 3
	{BF_ArcMain_BF0, {128, 177, 68,  79}, { 60,  79}}, //3 idle 4
	
	{BF_ArcMain_BF1, {  0,  48,  77,  79}, { 64,  79}}, //4 left 1
	{BF_ArcMain_BF1, {  128, 48,  74,  79}, { 62,  79}}, //5 left 2
	
	{BF_ArcMain_BF1, {  0, 185,  60,  71}, { 56,  71}}, //6 down 1
	{BF_ArcMain_BF1, {  128,   182,  57,  74}, { 53,  74}}, //7 down 2
	
	{BF_ArcMain_BF2, { 0,   169,  54,  87}, { 50,  87}}, //8 up 1
	{BF_ArcMain_BF2, {  128,  172,  57,  84}, { 52,  84}}, //9 up 2
	
	{BF_ArcMain_BF2, {  0,   49, 68,  78}, { 48, 78}}, //10 right 1
	{BF_ArcMain_BF2, {  128,  48, 70,  79}, { 54, 79}}, //11 right 2
	
	{BF_ArcMain_BF5, {  0,   128,  128,  128}, { 60, 120}}, //12 left miss 1
	{BF_ArcMain_BF5, { 128,   128,  128,  128}, { 60, 120}}, //13 left miss 2
	
	{BF_ArcMain_BF5, {  0,   0,  128,  128}, { 60, 120}}, //14 down miss 1
	{BF_ArcMain_BF5, { 128, 0,  128,  128}, { 60, 120}}, //15 down miss 2
	
	{BF_ArcMain_BF6, {  0,   128,  128,  128}, { 60, 120}}, //16 up miss 1
	{BF_ArcMain_BF6, { 128,   128,  128,  128}, { 60, 120}}, //17 up miss 2
	
	{BF_ArcMain_BF6, {  0, 0,  128,  128}, { 60, 120}}, //18 right miss 1
	{BF_ArcMain_BF6, {128, 0, 128,  128}, { 60, 120}}, //19 right miss 2

	{BF_ArcMain_Dead0, {  0,   0, 85, 86}, { 53,  98}}, //20 dead0 0
	{BF_ArcMain_Dead0, {86,   5, 74, 74}, { 53,  98}}, //21 dead0 1
	{BF_ArcMain_Dead0, {  161, 9, 68, 67}, { 53,  98}}, //22 dead0 2
	{BF_ArcMain_Dead0, {0, 86, 85, 86}, { 53,  98}}, //23 dead0 3
	
	{BF_ArcDead_Dead1, {  0,   0,  85,  86}, { 53,  98}}, //24 dead1 0
	{BF_ArcDead_Dead1, {128,   0, 128, 128}, { 53,  98}}, //25 dead1 1
	{BF_ArcDead_Dead1, {  0, 128, 128, 128}, { 53,  98}}, //26 dead1 2
	{BF_ArcDead_Dead1, {128, 128, 128, 128}, { 53,  98}}, //27 dead1 3
	
	{BF_ArcDead_Dead2, {  0,   0, 128, 128}, { 53,  98}}, //28 dead2 body twitch 0
	{BF_ArcDead_Dead2, {128,   0, 128, 128}, { 53,  98}}, //29 dead2 body twitch 1
	{BF_ArcDead_Dead2, {  0, 128, 128, 128}, { 53,  98}}, //30 dead2 balls twitch 0
	{BF_ArcDead_Dead2, {128, 128, 128, 128}, { 53,  98}}, //31 dead2 balls twitch 1

	{BF_ArcMain_Alt0, {  0,   0, 68,  74}, { 66,  73}}, //32 idle 1
	{BF_ArcMain_Alt0, {68,  0, 67,  77}, { 63,  76}}, //33 idle 2
	{BF_ArcMain_Alt0, {  135, 0, 67,  79}, { 60,  79}}, //34 idle 3
	{BF_ArcMain_Alt0, {0, 75, 68,  79}, { 60,  79}}, //35 idle 4
	
	{BF_ArcMain_Alt0, {  68,  78,  78,  79}, { 64,  79}}, //36 left 1
	{BF_ArcMain_Alt0, {  146, 80,  74,  79}, { 62,  79}}, //37 left 2
	
	{BF_ArcMain_Alt0, {  0, 155,  61,  70}, { 56,  71}}, //38 down 1
	{BF_ArcMain_Alt0, {  61,   157,  57,  74}, { 53,  74}}, //39 down 2
	
	{BF_ArcMain_Alt1, { 0,   0,  52,  88}, { 50,  87}}, //40 up 1
	{BF_ArcMain_Alt1, {  52,  0,  57,  84}, { 52,  84}}, //41 up 2
	
	{BF_ArcMain_Alt0, {  118,   159, 66,  78}, { 48, 78}}, //42 right 1
	{BF_ArcMain_Alt0, {  187,  160, 69,  79}, { 54, 79}}, //43 right 2
	
	{BF_ArcMain_Alt1, {  118,   79,  74,  79}, {64,  79}}, //44 left miss 1
	{BF_ArcMain_Alt1, { 0,   162,  81,  79}, { 62,  79}}, //45 left miss 2
	
	{BF_ArcMain_Alt1, {  0,   88,  57,  74}, { 56,  71}}, //46 down miss 1
	{BF_ArcMain_Alt1, { 57, 84,  61,  72}, { 53,  74}}, //47 down miss 2
	
	{BF_ArcMain_Alt1, {  81,   158,  57,  84}, { 50,  87}}, //48 up miss 1
	{BF_ArcMain_Alt1, { 138,   158,  52,  89}, { 52,  84}}, //49 up miss 2
	
	{BF_ArcMain_Alt1, {  109, 0,  69,  79}, { 48, 78}}, //50 right miss 1
	{BF_ArcMain_Alt1, {178, 0, 78,  77}, { 54, 79}}, //51 right miss 2
};

static const Animation char_bf_anim[PlayerAnim_Max] = {
	{2, (const u8[]){ 0,  1,  2,  3, ASCR_BACK, 1}},     //CharAnim_Idle
	{2, (const u8[]){ 4,  5, ASCR_BACK, 1}},             //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_LeftAlt
	{2, (const u8[]){ 6,  7, ASCR_BACK, 1}},             //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_DownAlt
	{2, (const u8[]){ 8, 9, ASCR_BACK, 1}},             //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_UpAlt
	{2, (const u8[]){ 10,  11, ASCR_BACK, 1}},             //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_RightAlt
	
	{1, (const u8[]){ 4, 12, 12, 13, ASCR_BACK, 1}},     //PlayerAnim_LeftMiss
	{1, (const u8[]){ 6, 14, 14, 15, ASCR_BACK, 1}},     //PlayerAnim_DownMiss
	{1, (const u8[]){ 8, 16, 16, 17, ASCR_BACK, 1}},     //PlayerAnim_UpMiss
	{1, (const u8[]){10, 18, 18, 19, ASCR_BACK, 1}},     //PlayerAnim_RightMiss
	
	{5, (const u8[]){20, 20, 20, 20, 20, 20, 20, 20, 20, ASCR_CHGANI, PlayerAnim_Dead1}}, //PlayerAnim_Dead0
	{5, (const u8[]){20, ASCR_REPEAT}},                   //PlayerAnim_Dead1
	{3, (const u8[]){20, ASCR_CHGANI, PlayerAnim_Dead3}}, //PlayerAnim_Dead2
	{3, (const u8[]){20, ASCR_REPEAT}},                   //PlayerAnim_Dead3
	{3, (const u8[]){20, ASCR_CHGANI, PlayerAnim_Dead3}}, //PlayerAnim_Dead4
	{3, (const u8[]){20, ASCR_CHGANI, PlayerAnim_Dead3}}, //PlayerAnim_Dead5
	
	{10, (const u8[]){23, 23, 23, ASCR_BACK, 1}}, //PlayerAnim_Dead4
	{ 3, (const u8[]){21, 22, 23, ASCR_REPEAT}},  //PlayerAnim_Dead5
};

static const Animation char_bf_animb[PlayerAnim_Max] = {
	{2, (const u8[]){ 32,  33,  34,  35, ASCR_BACK, 1}},     //CharAnim_Idle
	{2, (const u8[]){ 36,  37, ASCR_BACK, 1}},             //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_LeftAlt
	{2, (const u8[]){ 38,  39, ASCR_BACK, 1}},             //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_DownAlt
	{2, (const u8[]){ 40, 41, ASCR_BACK, 1}},             //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_UpAlt
	{2, (const u8[]){ 42,  43, ASCR_BACK, 1}},             //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_RightAlt
	
	{1, (const u8[]){ 36, 44, 44, 45, ASCR_BACK, 1}},     //PlayerAnim_LeftMiss
	{1, (const u8[]){ 38, 46, 46, 47, ASCR_BACK, 1}},     //PlayerAnim_DownMiss
	{1, (const u8[]){ 40, 48, 48, 49, ASCR_BACK, 1}},     //PlayerAnim_UpMiss
	{1, (const u8[]){42, 50, 50, 51, ASCR_BACK, 1}},     //PlayerAnim_RightMiss
	
	{5, (const u8[]){20, 20, 20, 20, 20, 20, 20, 20, 20, ASCR_CHGANI, PlayerAnim_Dead1}}, //PlayerAnim_Dead0
	{5, (const u8[]){20, ASCR_REPEAT}},                   //PlayerAnim_Dead1
	{3, (const u8[]){20, ASCR_CHGANI, PlayerAnim_Dead3}}, //PlayerAnim_Dead2
	{3, (const u8[]){20, ASCR_REPEAT}},                   //PlayerAnim_Dead3
	{3, (const u8[]){20, ASCR_CHGANI, PlayerAnim_Dead3}}, //PlayerAnim_Dead4
	{3, (const u8[]){20, ASCR_CHGANI, PlayerAnim_Dead3}}, //PlayerAnim_Dead5
	
	{10, (const u8[]){23, 23, 23, ASCR_BACK, 1}}, //PlayerAnim_Dead4
	{ 3, (const u8[]){21, 22, 23, ASCR_REPEAT}},  //PlayerAnim_Dead5
};


//Boyfriend player functions
void Char_BF_SetFrame(void *user, u8 frame)
{
	Char_BF *this = (Char_BF*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_bf_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

void Char_BF_Tick(Character *character)
{
	Char_BF *this = (Char_BF*)character;
	
	//Perform idle dance
	if ((character->pad_held & (INPUT_LEFT | INPUT_DOWN | INPUT_UP | INPUT_RIGHT)) == 0)
		Character_PerformIdle(character);
	
	//Retry screen
	if (character->animatable.anim >= PlayerAnim_Dead3)
	{
		//Tick skull fragments
		if (this->skull_scale)
		{
			SkullFragment *frag = this->skull;
			for (size_t i = 0; i < COUNT_OF_MEMBER(Char_BF, skull); i++, frag++)
			{
				//Draw fragment
				RECT frag_src = {
					(i & 1) ? 112 : 96,
					(i >> 1) << 4,
					16,
					16
				};
				fixed_t skull_dim = (FIXED_DEC(16,1) * this->skull_scale) >> 6;
				fixed_t skull_rad = skull_dim >> 1;
				RECT_FIXED frag_dst = {
					character->x + (((fixed_t)frag->x << FIXED_SHIFT) >> 3) - skull_rad - stage.camera.x,
					character->y + (((fixed_t)frag->y << FIXED_SHIFT) >> 3) - skull_rad - stage.camera.y,
					skull_dim,
					skull_dim,
				};
				Stage_DrawTex(&this->tex_retry, &frag_src, &frag_dst, FIXED_MUL(stage.camera.zoom, stage.bump));
				
				//Move fragment
				frag->x += frag->xsp;
				frag->y += ++frag->ysp;
			}
			
			//Decrease scale
			this->skull_scale--;
		}
		
		//Draw input options
		u8 input_scale = 16 - this->skull_scale;
		if (input_scale > 16)
			input_scale = 0;
		
		RECT button_src = {
			 0, 96,
			16, 16
		};
		RECT_FIXED button_dst = {
			character->x - FIXED_DEC(32,1) - stage.camera.x,
			character->y - FIXED_DEC(88,1) - stage.camera.y,
			(FIXED_DEC(16,1) * input_scale) >> 4,
			FIXED_DEC(16,1),
		};
		
		//Cross - Retry
		Stage_DrawTex(&this->tex_retry, &button_src, &button_dst, FIXED_MUL(stage.camera.zoom, stage.bump));
		
		//Circle - Blueball
		button_src.x = 16;
		button_dst.y += FIXED_DEC(56,1);
		Stage_DrawTex(&this->tex_retry, &button_src, &button_dst, FIXED_MUL(stage.camera.zoom, stage.bump));
		
		//Draw 'RETRY'
		u8 retry_frame;
		
		if (character->animatable.anim == PlayerAnim_Dead6)
		{
			//Selected retry
			retry_frame = 2 - (this->retry_bump >> 3);
			if (retry_frame >= 3)
				retry_frame = 0;
			if (this->retry_bump & 2)
				retry_frame += 3;
			
			if (++this->retry_bump == 0xFF)
				this->retry_bump = 0xFD;
		}
		else
		{
			//Idle
			retry_frame = 1 +  (this->retry_bump >> 2);
			if (retry_frame >= 3)
				retry_frame = 0;
			
			if (++this->retry_bump >= 55)
				this->retry_bump = 0;
		}
		
		RECT retry_src = {
			(retry_frame & 1) ? 48 : 0,
			(retry_frame >> 1) << 5,
			48,
			32
		};
		RECT_FIXED retry_dst = {
			character->x -  FIXED_DEC(7,1) - stage.camera.x,
			character->y - FIXED_DEC(92,1) - stage.camera.y,
			FIXED_DEC(48,1),
			FIXED_DEC(32,1),
		};
		Stage_DrawTex(&this->tex_retry, &retry_src, &retry_dst, FIXED_MUL(stage.camera.zoom, stage.bump));
	}
	
	//Animate and draw character
	if (stage.stage_id == StageId_1_1 && stage.song_step < 1538)
		Animatable_Animate(&character->animatable, (void*)this, Char_BF_SetFrame);
	else
		Animatable_Animate(&character->animatableb, (void*)this, Char_BF_SetFrame);
	Character_Draw(character, &this->tex, &char_bf_frame[this->frame]);
}

void Char_BF_SetAnim(Character *character, u8 anim)
{
	Char_BF *this = (Char_BF*)character;
	
	//Perform animation checks
	switch (anim)
	{
		case PlayerAnim_Dead0:
			//Begin reading dead.arc and adjust focus
			this->arc_dead = IO_AsyncReadFile(&this->file_dead_arc);
			character->focus_x = FIXED_DEC(0,1);
			character->focus_y = FIXED_DEC(-40,1);
			character->focus_zoom = FIXED_DEC(125,100);
			break;
		case PlayerAnim_Dead2:
			//Unload main.arc
			Mem_Free(this->arc_main);
			this->arc_main = this->arc_dead;
			this->arc_dead = NULL;
			
			//Find dead.arc files
			const char **pathp = (const char *[]){
				"retry.tim", //BF_ArcDead_Retry
				NULL
			};
			IO_Data *arc_ptr = this->arc_ptr;
			for (; *pathp != NULL; pathp++)
				*arc_ptr++ = Archive_Find(this->arc_main, *pathp);
			
			//Load retry art
			Gfx_LoadTex(&this->tex_retry, this->arc_ptr[BF_ArcDead_Retry], 0);
			break;
	}
	
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
	Animatable_SetAnim(&character->animatableb, anim);
	Character_CheckStartSing(character);
}

void Char_BF_Free(Character *character)
{
	Char_BF *this = (Char_BF*)character;
	
	//Free art
	Mem_Free(this->arc_main);
	Mem_Free(this->arc_dead);
}

Character *Char_BF_New(fixed_t x, fixed_t y)
{
	//Allocate boyfriend object
	Char_BF *this = Mem_Alloc(sizeof(Char_BF));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_BF_New] Failed to allocate boyfriend object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_BF_Tick;
	this->character.set_anim = Char_BF_SetAnim;
	this->character.free = Char_BF_Free;
	
	Animatable_Init(&this->character.animatable, char_bf_anim);
	Animatable_Init(&this->character.animatableb, char_bf_animb);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = CHAR_SPEC_MISSANIM;
	
	this->character.health_i = 0;
	
	this->character.focus_x = FIXED_DEC(-50,1);
	this->character.focus_y = FIXED_DEC(-105,1);
	this->character.focus_zoom = FIXED_DEC(1,1);
	
	//Load art
	this->arc_main = IO_Read("\\CHAR\\JB.ARC;1");
	this->arc_dead = NULL;
	IO_FindFile(&this->file_dead_arc, "\\CHAR\\BFDEAD.ARC;1");
	
	const char **pathp = (const char *[]){
		"jb0.tim",   //BF_ArcMain_BF0
		"jb1.tim",   //BF_ArcMain_BF1
		"jb2.tim",   //BF_ArcMain_BF2
		"jb3.tim",   //BF_ArcMain_BF5
		"jb4.tim",   //BF_ArcMain_BF6
		"alt0.tim",   //BF_ArcMain_BF5
		"alt1.tim",   //BF_ArcMain_BF6
		"jbdead.tim", //BF_ArcMain_Dead0
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find(this->arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	//Initialize player state
	this->retry_bump = 0;
	
	//Copy skull fragments
	memcpy(this->skull, char_bf_skull, sizeof(char_bf_skull));
	this->skull_scale = 64;
	
	SkullFragment *frag = this->skull;
	for (size_t i = 0; i < COUNT_OF_MEMBER(Char_BF, skull); i++, frag++)
	{
		//Randomize trajectory
		frag->xsp += RandomRange(-4, 4);
		frag->ysp += RandomRange(-2, 2);
	}
	
	return (Character*)this;
}
