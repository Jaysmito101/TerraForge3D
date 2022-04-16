#pragma once


namespace TerraForge3D
{
	/*
	* This is just the same as GLFW Keycodes just as a enum rather than macros
	*/
	enum KeyCode 
	{
		KeyCode_Unknown           = 0, /* This has been change from -1 in GLFW to 0 in TerraForge3D for its state to be easily stored in an array. */
		KeyCode_Space             = 32,
		KeyCode_Apostrophe        = 39,  /* ' */
		KeyCode_Comma             = 44,  /* , */
		KeyCode_Minus             = 45,  /* - */
		KeyCode_Period            = 46,  /* . */
		KeyCode_Slash             = 47,  /* / */
		KeyCode_0                 = 48,
		KeyCode_1                 = 49,
		KeyCode_2                 = 50,
		KeyCode_3                 = 51,
		KeyCode_4                 = 52,
		KeyCode_5                 = 53,
		KeyCode_6                 = 54,
		KeyCode_7                 = 55,
		KeyCode_8                 = 56,
		KeyCode_9                 = 57,
		KeyCode_Semicolon         = 59,  /* ; */
		KeyCode_Equal             = 61,  /* = */
		KeyCode_A                 = 65,
		KeyCode_B                 = 66,
		KeyCode_C                 = 67,
		KeyCode_D                 = 68,
		KeyCode_E                 = 69,
		KeyCode_F                 = 70,
		KeyCode_G                 = 71,
		KeyCode_H                 = 72,
		KeyCode_I                 = 73,
		KeyCode_J                 = 74,
		KeyCode_K                 = 75,
		KeyCode_L                 = 76,
		KeyCode_M                 = 77,
		KeyCode_N                 = 78,
		KeyCode_O                 = 79,
		KeyCode_P                 = 80,
		KeyCode_Q                 = 81,
		KeyCode_R                 = 82,
		KeyCode_S                 = 83,
		KeyCode_T                 = 84,
		KeyCode_U                 = 85,
		KeyCode_V                 = 86,
		KeyCode_W                 = 87,
		KeyCode_X                 = 88,
		KeyCode_Y                 = 89,
		KeyCode_Z                 = 90,
		KeyCode_LeftBracket       = 91,  /* [ */
		KeyCode_Backslash         = 92,  /* \ */
		KeyCode_RightBracket      = 93,  /* ] */
		KeyCode_GraveAccent       = 96,  /* ` */
		KeyCode_World1            = 161, /* non-US #1 */
		KeyCode_World2            = 162, /* non-US #2 */
		KeyCode_Escape            = 256,
		KeyCode_Enter             = 257,
		KeyCode_Tab               = 258,
		KeyCode_Backspace         = 259,
		KeyCode_Insert            = 260,
		KeyCode_Delete            = 261,
		KeyCode_Right             = 262,
		KeyCode_Left              = 263,
		KeyCode_Down              = 264,
		KeyCode_Up                = 265,
		KeyCode_PageUp            = 266,
		KeyCode_PageDown          = 267,
		KeyCode_Home              = 268,
		KeyCode_End               = 269,
		KeyCode_CapsLock          = 280,
		KeyCode_ScrollLock        = 281,
		KeyCode_NumLock           = 282,
		KeyCode_PrintScreen       = 283,
		KeyCode_Pause             = 284,
		KeyCode_F1                = 290,
		KeyCode_F2                = 291,
		KeyCode_F3                = 292,
		KeyCode_F4                = 293,
		KeyCode_F5                = 294,
		KeyCode_F6                = 295,
		KeyCode_F7                = 296,
		KeyCode_F8                = 297,
		KeyCode_F9                = 298,
		KeyCode_F10               = 299,
		KeyCode_F11               = 300,
		KeyCode_F12               = 301,
		KeyCode_F13               = 302,
		KeyCode_F14               = 303,
		KeyCode_F15               = 304,
		KeyCode_F16               = 305,
		KeyCode_F17               = 306,
		KeyCode_F18               = 307,
		KeyCode_F19               = 308,
		KeyCode_F20               = 309,
		KeyCode_F21               = 310,
		KeyCode_F22               = 311,
		KeyCode_F23               = 312,
		KeyCode_F24               = 313,
		KeyCode_F25               = 314,
		KeyCode_Keypad0           = 320,
		KeyCode_Keypad1           = 321,
		KeyCode_Keypad2           = 322,
		KeyCode_Keypad3           = 323,
		KeyCode_Keypad4           = 324,
		KeyCode_Keypad5           = 325,
		KeyCode_Keypad6           = 326,
		KeyCode_Keypad7           = 327,
		KeyCode_Keypad8           = 328,
		KeyCode_Keypad9           = 329,
		KeyCode_KeyPadDecimal     = 330,
		KeyCode_KeypadDivide      = 331,
		KeyCode_KeypadMultiply    = 332,
		KeyCode_KeypadSubtract    = 333,
		KeyCode_KeypadAdd         = 334,
		KeyCode_KeypadEnter       = 335,
		KeyCode_KeypadEqual       = 336,
		KeyCode_LeftShift         = 340,
		KeyCode_LeftControl       = 341,
		KeyCode_LeftAlt           = 342,
		KeyCode_RightShift        = 344,
		KeyCode_RightControl      = 345,
		KeyCode_RightAlt          = 346,
		KeyCode_RightSuper        = 347,
		KeyCode_Menu              = 348
	};

	enum MouseButton
	{
		MouseButton_1        = 0,
		MouseButton_2        = 1,
		MouseButton_3        = 2,
		MouseButton_4        = 3,
		MouseButton_5        = 4,
		MouseButton_6        = 5,
		MouseButton_7        = 6,
		MouseButton_8        = 7,
		MouseButton_Last     = MouseButton_8,
		MouseButton_Left     = MouseButton_1,
		MouseButton_Right    = MouseButton_2,
		MouseButton_Middle   = MouseButton_3
	};
}
