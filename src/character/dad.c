/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "dad.h"

#include "../mem.h"
#include "../archive.h"
#include "../stage.h"
#include "../main.h"

//Dad character structure
enum
{
	Dad_ArcMain_Idle0,
	Dad_ArcMain_Idle1,
	Dad_ArcMain_Idle2,
	Dad_ArcMain_Idle3,
	Dad_ArcMain_Left1,
	Dad_ArcMain_Left2,
	Dad_ArcMain_Down1,
	Dad_ArcMain_Down2,
	Dad_ArcMain_Up1,
	Dad_ArcMain_Up2,
	Dad_ArcMain_Right1,
	Dad_ArcMain_Right2,
	Dad_ArcMain_Alt0,
	Dad_ArcMain_Alt1,
	Dad_ArcMain_Alt2,
	Dad_ArcMain_Alt3,
	Dad_ArcMain_Alt4,
	Dad_ArcMain_Alt5,
	
	Dad_Arc_Max,
};

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main;
	IO_Data arc_ptr[Dad_Arc_Max];
	
	Gfx_Tex tex;
	u8 frame, tex_id;
} Char_Dad;

//Dad character definitions
static const CharFrame char_dad_frame[] = {
	{Dad_ArcMain_Idle0, {  0,   0, 256, 256}, { 60, 250}}, //0 idle 1
	{Dad_ArcMain_Idle1, {0,   0, 256, 256}, { 60, 250}}, //1 idle 2
	{Dad_ArcMain_Idle2, {  0,   0, 256, 256}, { 60, 250}}, //2 idle 3
	{Dad_ArcMain_Idle3, {0,   0, 256, 256}, { 60, 250}}, //3 idle 4
	
	{Dad_ArcMain_Left1, {  0,   0,  256, 256}, { 60, 250}}, //4 left 1
	{Dad_ArcMain_Left2, { 0,   0,  256, 256}, { 60, 250}}, //5 left 2
	
	{Dad_ArcMain_Down1, {  0,   0, 256, 256}, { 60, 250}}, //6 down 1
	{Dad_ArcMain_Down2, {0,   0, 256, 256}, { 60, 250}}, //7 down 2
	
	{Dad_ArcMain_Up1, {  0,   0, 256, 256}, { 60, 250}}, //8 up 1
	{Dad_ArcMain_Up2, {0,   0, 256, 256}, { 60, 250}}, //9 up 2
	
	{Dad_ArcMain_Right1, {  0,   0, 256, 256}, { 60, 250}}, //10 right 1
	{Dad_ArcMain_Right2, {0,   0, 256, 256}, { 60, 250}}, //11 right 2

	{Dad_ArcMain_Alt0, {  0,   0, 144, 121}, { 76, 121}}, //0 idle 1
	{Dad_ArcMain_Alt0, {0,   121, 144, 123}, { 70, 123}}, //1 idle 2
	{Dad_ArcMain_Alt1, {  0,   0, 130, 126}, { 62, 126}}, //2 idle 3
	{Dad_ArcMain_Alt1, {0,   126, 126, 124}, { 58, 124}}, //3 idle 4
	
	{Dad_ArcMain_Alt2, {  0,   0,  174, 106}, { 106, 106}}, //4 left 1
	{Dad_ArcMain_Alt2, { 0,  107,  161, 113}, { 92, 113}}, //5 left 2
	
	{Dad_ArcMain_Alt1, {  130,   0, 80, 101}, { 28, 101}}, //6 down 1
	{Dad_ArcMain_Alt1, {134,   101, 89, 111}, { 40, 111}}, //7 down 2
	
	{Dad_ArcMain_Alt4, {  0,   0, 128, 144}, { 64, 144}}, //8 up 1
	{Dad_ArcMain_Alt5, {0,   0, 138, 136}, { 69, 136}}, //9 up 2
	
	{Dad_ArcMain_Alt3, {  0,   0, 126, 135}, { 28, 135}}, //10 right 1
	{Dad_ArcMain_Alt3, {141,   0, 115, 125}, { 35, 125}}, //11 right 2
};

static const Animation char_dad_anim[CharAnim_Max] = {
	{2, (const u8[]){ 0,  1,  2,  3, ASCR_BACK, 1}}, //CharAnim_Idle
	{2, (const u8[]){ 4,  5, ASCR_BACK, 1}},         //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_LeftAlt
	{2, (const u8[]){ 6,  7, ASCR_BACK, 1}},         //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_DownAlt
	{2, (const u8[]){ 8,  9, ASCR_BACK, 1}},         //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_UpAlt
	{2, (const u8[]){10, 11, ASCR_BACK, 1}},         //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_RightAlt
};

static const Animation char_dad_animb[CharAnim_Max] = {
	{2, (const u8[]){ 12,  13,  14,  15, ASCR_BACK, 1}}, //CharAnim_Idle
	{2, (const u8[]){ 16,  17, ASCR_BACK, 1}},         //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_LeftAlt
	{2, (const u8[]){ 18,  19, ASCR_BACK, 1}},         //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_DownAlt
	{2, (const u8[]){ 20,  21, ASCR_BACK, 1}},         //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_UpAlt
	{2, (const u8[]){22, 23, ASCR_BACK, 1}},         //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_RightAlt
};

//Dad character functions
void Char_Dad_SetFrame(void *user, u8 frame)
{
	Char_Dad *this = (Char_Dad*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_dad_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

void Char_Dad_Tick(Character *character)
{
	Char_Dad *this = (Char_Dad*)character;
	
	//Perform idle dance
	if ((character->pad_held & (INPUT_LEFT | INPUT_DOWN | INPUT_UP | INPUT_RIGHT)) == 0)
		Character_PerformIdle(character);
	
	//Animate and draw
	if (stage.stage_id == StageId_1_1 && stage.song_step < 1538)
		Animatable_Animate(&character->animatable, (void*)this, Char_Dad_SetFrame);
	else
		Animatable_Animate(&character->animatableb, (void*)this, Char_Dad_SetFrame);
	Character_Draw(character, &this->tex, &char_dad_frame[this->frame]);
}

void Char_Dad_SetAnim(Character *character, u8 anim)
{
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
	Animatable_SetAnim(&character->animatableb, anim);
	Character_CheckStartSing(character);
}

void Char_Dad_Free(Character *character)
{
	Char_Dad *this = (Char_Dad*)character;
	
	//Free art
	Mem_Free(this->arc_main);
}

Character *Char_Dad_New(fixed_t x, fixed_t y)
{
	//Allocate dad object
	Char_Dad *this = Mem_Alloc(sizeof(Char_Dad));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_Dad_New] Failed to allocate dad object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_Dad_Tick;
	this->character.set_anim = Char_Dad_SetAnim;
	this->character.free = Char_Dad_Free;
	
	Animatable_Init(&this->character.animatable, char_dad_anim);
	Animatable_Init(&this->character.animatableb, char_dad_animb);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = 0;
	
	this->character.health_i = 1;
	
	this->character.focus_x = FIXED_DEC(65,1);
	this->character.focus_y = FIXED_DEC(-90,1);
	this->character.focus_zoom = FIXED_DEC(1,1);
	
	//Load art
	this->arc_main = IO_Read("\\CHAR\\SKELE.ARC;1");
	
	const char **pathp = (const char *[]){
		"idle0.tim", //Dad_ArcMain_Idle0
		"idle1.tim", //Dad_ArcMain_Idle1
		"idle2.tim", //Dad_ArcMain_Idle0
		"idle3.tim", //Dad_ArcMain_Idle1
		"left1.tim",  //Dad_ArcMain_Left
		"left2.tim",  //Dad_ArcMain_Left
		"down1.tim",  //Dad_ArcMain_Down
		"down2.tim",  //Dad_ArcMain_Down
		"up1.tim",    //Dad_ArcMain_Up
		"up2.tim",    //Dad_ArcMain_Up
		"right1.tim", //Dad_ArcMain_Right
		"right2.tim", //Dad_ArcMain_Right
		"alt0.tim", //Dad_ArcMain_Idle0
		"alt1.tim", //Dad_ArcMain_Idle1
		"alt2.tim", //Dad_ArcMain_Idle0
		"alt3.tim", //Dad_ArcMain_Idle1
		"alt4.tim", //Dad_ArcMain_Idle0
		"alt5.tim", //Dad_ArcMain_Idle1
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find(this->arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	return (Character*)this;
}
