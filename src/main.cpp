#include <vector>
#include <cstdint>

using namespace geode::prelude;

#define PATCH(x, y) (void) Mod::get()->patch(reinterpret_cast<void*>(geode::base::get() + x), y);

// constexpr float max_object_position = 240000.0f; // = 0x486a6000 and 0x486a6780
#define MAX_POSITIONS_AS_BYTEARRAYS\
	constexpr float max_object_position = 999'999'999.0f;\
	constexpr float max_camera_position = max_object_position + 30.0f;\
	std::vector patch_a = float_to_arr(max_object_position);\
	std::vector patch_b = float_to_arr(max_camera_position);\

constexpr std::vector<std::uint8_t> float_to_arr(float x) {
  union {
	float a;
	std::uint8_t b[4];
  } y = {x};

  return {y.b, y.b + 4};
}

$on_mod(Loaded) {
	#if defined(GEODE_IS_ARM_MAC)
	// on arm, floats are embedded into the instructions themselves
	// max_pos = 0x4e6e 6b28 (blame float rounding)
	// to save time, we only patch the exponent

	// byte vector by qimiko/zmx: https://github.com/qimiko
	// you probably shouldn't change it since it determines the float that gets loaded into the game
	// yknow, the float that determines the maximum length of the level editor?
	// yeah
	std::vector<std::uint8_t> patch_bytes{0xc8, 0xcd, 0xa9, 0x72};

	// for macOS ARM, you need to find ? f0 8c 52 ? 0d a9 72
	// preferably use the `? 0d a9 72` search pattern to find all instances of 240000.0f
	// addresses based on https://files.catbox.moe/wvfp8j.png [this search pattern accomodates both floats]

	/*
	08 f0 8c 52  mov  w8, #0x6780
	48 0d a9 72  movk w8, #0x486a, LSL #16
	*/

	PATCH(0x2b32c, patch_bytes); // EditorUI::constrainGameLayerPosition => 0x4e6e6780

	/*
	08 00 8c 52  mov  w8, #0x6000
	48 0d a9 72  movk w8, #0x486a, LSL #16
	*/

	PATCH(0x3481c, patch_bytes);
	PATCH(0x351cc, patch_bytes);

	// these patches sorta exist, i guess. apparently patching anything for DrawGridLayer breaks on windows
	// either that, or TheSillyDoggo just took a lot of shortcuts to her approach.
	// oh well.
	// AS OF MARCH 25, 2025: COMMENTED OUT, BREAKS DRAWGRIDLAYER ON MACOS ARM (?)
	/*
	PATCH(0xda150, patch_bytes); // DrawGridLayer::draw => 0x4e6e6780
	PATCH(0xda914, patch_bytes); // DrawGridLayer::draw => 0x4e6e6780
	PATCH(0xdabcc, patch_bytes); // DrawGridLayer::draw => 0x4e6e6780
	PATCH(0xdad60, patch_bytes); // DrawGridLayer::draw => 0x4e6e6780
	PATCH(0xdae20, patch_bytes); // DrawGridLayer::draw => 0x4e6e6780
	PATCH(0xdae64, patch_bytes); // DrawGridLayer::draw => 0x4e6e6780
	*/

	// unknown functions, but they have the same bytes as the previous ones
	PATCH(0x394c0, patch_bytes);
	PATCH(0x39a5c, patch_bytes);
	PATCH(0x3e0e4, patch_bytes);
	PATCH(0x462f8, patch_bytes);
	PATCH(0x46a94, patch_bytes);
	#elif defined(GEODE_IS_INTEL_MAC)
	// for macOS Intel, you need to find 0x486a6780 [240030.0f] and 0x486a6000 [240000.0f]
	// addresses based on https://files.catbox.moe/8su33t.png [for 240030]
	// and https://files.catbox.moe/rzpj6t.png [for 240000]

	MAX_POSITIONS_AS_BYTEARRAYS

	// max object position (used in too many functions to name)
	PATCH(0x7fdf4c, patch_a);

	// max camera position (only used in EditorUI::constrainGameLayerPosition)
	PATCH(0x7fdf34, patch_b);

	// a weird one
	PATCH(0x7fe110, patch_a);
	#elif defined(GEODE_IS_ANDROID64)
	// for Android 64-bit, you need to find 0x486a6780 [240030.0f] and 0x486a6000 [240000.0f]
	// addresses based on https://files.catbox.moe/3796mo.png [for 240030]
	// and https://files.catbox.moe/ujja7q.png [for 240000]

	MAX_POSITIONS_AS_BYTEARRAYS

	// DrawGridLayer::draw (uses 0x486a6000)
	PATCH(0x75d740 - 0x100000, patch_a);

	// EditorUI::getLimitedPosition (uses 0x486a6000)
	PATCH(0x7773c4 - 0x100000, patch_a);

	// EditorUI::onCreateObject (uses 0x486a6000)
	PATCH(0x7a7f44 - 0x100000, patch_a);

	// EditorUI::constrainGameLayerPosition (uses 0x486a6780)
	PATCH(0x77790c - 0x100000, patch_b);
	#elif defined(GEODE_IS_ANDROID32)
	// for Android 32-bit, you need to find 0x486a6780 [240030.0f] and 0x486a6000 [240000.0f]
	// addresses based on https://files.catbox.moe/bnztgs.png [for 240030]
	// and https://files.catbox.moe/tfgebw.png [for 240000]

	MAX_POSITIONS_AS_BYTEARRAYS

	// DrawGridLayer::draw (uses 0x486a6000)
	// why is it split into three places?
	PATCH(0x372928 - 0x10000, patch_a);
	PATCH(0x37304c - 0x10000, patch_a);
	PATCH(0x373558 - 0x10000, patch_a);

	// EditorUI::getLimitedPosition (uses 0x486a6000)
	PATCH(0x382bd8 - 0x10000, patch_a);

	// EditorUI::onCreateObject (uses 0x486a6000)
	PATCH(0x3a5364 - 0x10000, patch_a);

	// EditorUI::constrainGameLayerPosition (uses 0x486a6780)
	PATCH(0x382f7c - 0x10000, patch_b);
	#elif defined(GEODE_IS_WINDOWS)
	// for Windows, you need to find 0x486a6780 [240030.0f] and 0x486a6000 [240000.0f]
	// addresses based on https://files.catbox.moe/joabt9.png [for 240030]
	// and https://files.catbox.moe/pjzqho.png [for 240000]

	MAX_POSITIONS_AS_BYTEARRAYS

	/*
	EditorUI::onCreateObject,
	EditorUI::moveObject,
	EditorUI::getLimitedPosition,
	draw:1402dbbdd(R), draw:1402dc78b(R), draw:1402dc7d7(R), draw:1402dc9cd(R), draw:1402dcbcc(R), draw:1402dcc22(R)
	(all of them use 0x486a6000)
	*/
	PATCH(0x607ca0, patch_a);

	// EditorUI::constrainGameLayerPosition (uses 0x486a6780)
	PATCH(0x607ca4, patch_b);
	#else
	#warning Unsupported platform for this mod.
	// I DO NOT GIVE CONSENT FOR THIS MOD TO BE PORTED TO IOS, FLERO CLIENT, OR SIMILAR.
	// ANY ATTEMPT TO CONTACT ME FOR PERMISSION TO PORT THIS MOD TO IOS WILL BE SWIFTLY IGNORED.
	// GO FIND THE ADDRESSES YOURSELVES, COWARDS.
	// -RayDeeUx
	geode::log::info("UNSUPPORTED PLATFORM DETECTED, LOSER! GRANDEDITOREXTENDER IS ILLEGITIMATE AND WILL NOT BE ALLOWED FOR USE ON IOS. PLEASE DISABLE THE MOD TO CONTINUE WITH YOUR DAY.");
	#endif
}