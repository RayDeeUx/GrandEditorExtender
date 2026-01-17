#include <vector>
#include <cstdint>

using namespace geode::prelude;

#define PATCH(x, y) (void) Mod::get()->patch(reinterpret_cast<void*>(geode::base::get() + x), y);

// constexpr float maxObjPos = 240000.0f; // = 0x486a6000
// constexpr float maxCamPos = 240030.0f; // = 0x486a6780
// constexpr float maxCtrPos = 999999.0f; // = 0x497423f0

// values determined by using qimiko's original 999'999'999.0f value
// and then improving them using https://www.h-schmidt.net/FloatConverter/IEEE754.html
// to avoid floating point precision loss
#define MAX_POSITIONS_AS_BYTEARRAYS\
	constexpr float maxObjPos = 1'000'000'000.0f;\
	constexpr float maxCamPos = 1'000'000'064.0f;\
	constexpr float maxCtrPos = 1'000'000'000.0f;\
	std::vector objPosPatch = floatToByteArray(maxObjPos);\
	std::vector cameraPosPatch = floatToByteArray(maxCamPos);\
	std::vector maxCtrPosPatch = floatToByteArray(maxCtrPos);
	
#define ANDROID64_BASE 0x100000
#define ANDROID32_BASE 0x10000

constexpr std::vector<std::uint8_t> floatToByteArray(float x) {
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
	// std::vector<std::uint8_t> patch_bytes{0xc8, 0xcd, 0xa9, 0x72};
	// ok so it turns out it had to get changed anyway because it was borked

	// Hello, hiimjasmine00 here. Since the float actually encompasses two instructions, we need to patch both of them.
	// The maximum object position will be 0x4e6e6b28, and the maximum camera position will be 0x4e6e6b29.
	// Also, some registers are different, so we need to change the first byte of the instruction to match the register.
	std::vector<std::uint8_t> w8_objPosPatch{0x08, 0x65, 0x8d, 0x52, 0xc8, 0xcd, 0xa9, 0x72}; // 0x4e6e6b28
	std::vector<std::uint8_t> w8_camPosPatch{0x28, 0x65, 0x8d, 0x52, 0xc8, 0xcd, 0xa9, 0x72}; // 0x4e6e6b29
	std::vector<std::uint8_t> w9_objPosPatch{0x09, 0x65, 0x8d, 0x52, 0xc9, 0xcd, 0xa9, 0x72}; // 0x4e6e6b28
	std::vector<std::uint8_t> w25_objPosPatch{0x19, 0x65, 0x8d, 0x52, 0xd9, 0xcd, 0xa9, 0x72}; // 0x4e6e6b28
	std::vector<std::uint8_t> w27_objPosPatch{0x1b, 0x65, 0x8d, 0x52, 0xdb, 0xcd, 0xa9, 0x72}; // 0x4e6e6b28
	// Alright, Jasmine out.

	// for macOS ARM, you need to find ? f0 8c 52 ? 0d a9 72
	// preferably use the `? 0d a9 72` search pattern to find all instances of 240000.0f
	// addresses based on https://files.catbox.moe/wvfp8j.png [this search pattern accomodates both floats]

	// Ghidra 12.0 update: searching for the ARM instructions from Program Text instead (Cmd + F)
	// then check instructions in https://xfreetool.com/en/armconverter
	// also patch from the address that has the `mov [register], #0x[MEMY]` instruction

	/*
	08 f0 8c 52  mov  w8, #0x6780
	48 0d a9 72  movk w8, #0x486a, LSL #16
	*/

	PATCH(0x2b328, w8_camPosPatch); // EditorUI::constrainGameLayerPosition => 0x4e6e6b29

	/*
	08 00 8c 52  mov  w8, #0x6000
	48 0d a9 72  movk w8, #0x486a, LSL #16
	*/

	PATCH(0x34818, w8_objPosPatch);
	PATCH(0x351c8, w8_objPosPatch);

	// these patches sorta exist, i guess. apparently patching anything for DrawGridLayer breaks on windows
	// either that, or TheSillyDoggo just took a lot of shortcuts to her approach.
	// oh well.
	PATCH(0xda024, w25_objPosPatch); // DrawGridLayer::draw => 0x4e6e6b28
	PATCH(0xda14c, w8_objPosPatch); // DrawGridLayer::draw => 0x4e6e6b28
	PATCH(0xda910, w8_objPosPatch); // DrawGridLayer::draw => 0x4e6e6b28
	PATCH(0xdabc8, w8_objPosPatch); // DrawGridLayer::draw => 0x4e6e6b28
	PATCH(0xdad5c, w8_objPosPatch); // DrawGridLayer::draw => 0x4e6e6b28
	PATCH(0xdae1c, w8_objPosPatch); // DrawGridLayer::draw => 0x4e6e6b28
	PATCH(0xdae60, w8_objPosPatch); // DrawGridLayer::draw => 0x4e6e6b28

	// unknown functions, but they have the same bytes as the previous ones
	PATCH(0x394bc, w8_objPosPatch);
	PATCH(0x39a58, w8_objPosPatch);
	PATCH(0x3e0e0, w8_objPosPatch);
	PATCH(0x43adc, w9_objPosPatch);
	PATCH(0x462f4, w8_objPosPatch);
	PATCH(0x467bc, w27_objPosPatch);
	PATCH(0x46a90, w8_objPosPatch);

	// EditorUI::getGroupCenter => 0x4e6e6b28
	std::vector<std::uint8_t> w8_maxCtrPosPatch{0x08, 0x65, 0x8d, 0x52, 0xc8, 0xcd, 0xa9, 0x72}; // 0x4e6e6b28
	/*
	08 7e 84 52		mov		w8, #0x23f0
	88 2e a9 72		movk	w8, #0x4974, LSL #16
	*/
	PATCH(0x36ed4, w8_maxCtrPosPatch);
	PATCH(0x36fd8, w8_maxCtrPosPatch);
	#elif defined(GEODE_IS_INTEL_MAC)
	// for macOS Intel, you need to find 0x486a6780 [240030.0f] and 0x486a6000 [240000.0f]
	// addresses based on https://files.catbox.moe/8su33t.png [for 240030]
	// and https://files.catbox.moe/rzpj6t.png [for 240000]
	// EditorUI::getGroupCenter UPDATE: also find 999999.f!
	// in Ghidra 12.0 and newer, just type the floats in manually.

	MAX_POSITIONS_AS_BYTEARRAYS

	// max object position (used in too many functions to name)
	PATCH(0x7fdf4c, objPosPatch);

	// max camera position (only used in EditorUI::constrainGameLayerPosition)
	PATCH(0x7fdf34, cameraPosPatch);

	// a weird one
	PATCH(0x7fe110, objPosPatch);

	// max object center (used in EditorUI::getGroupCenter)
	PATCH(0x7fe074, maxCtrPosPatch);
	PATCH(0x7fe120, maxCtrPosPatch);
	PATCH(0x7fe124, maxCtrPosPatch);
	#elif defined(GEODE_IS_ANDROID64)
	// for Android 64-bit, you need to find 0x486a6780 [240030.0f] and 0x486a6000 [240000.0f]
	// addresses based on https://files.catbox.moe/3796mo.png [for 240030]
	// and https://files.catbox.moe/ujja7q.png [for 240000]
	// EditorUI::getGroupCenter UPDATE: also find 999999.f!
	// in Ghidra 12.0 and newer, just type the floats in manually.

	MAX_POSITIONS_AS_BYTEARRAYS

	// DrawGridLayer::draw (uses 0x486a6000)
	PATCH(0x75d740 - ANDROID64_BASE, objPosPatch);

	// EditorUI::getLimitedPosition (uses 0x486a6000)
	PATCH(0x7773c4 - ANDROID64_BASE, objPosPatch);

	// EditorUI::onCreateObject (uses 0x486a6000)
	PATCH(0x7a7f44 - ANDROID64_BASE, objPosPatch);

	// EditorUI::constrainGameLayerPosition (uses 0x486a6780)
	PATCH(0x77790c - ANDROID64_BASE, cameraPosPatch);

	// EditorUI::getGroupCenter (uses 0x497423f0)
	PATCH(0x778034 - ANDROID64_BASE, maxCtrPosPatch);
	#elif defined(GEODE_IS_ANDROID32)
	// for Android 32-bit, you need to find 0x486a6780 [240030.0f] and 0x486a6000 [240000.0f]
	// addresses based on https://files.catbox.moe/bnztgs.png [for 240030]
	// and https://files.catbox.moe/tfgebw.png [for 240000]
	// EditorUI::getGroupCenter UPDATE: also find 999999.f!
	// in Ghidra 12.0 and newer, just type the floats in manually.

	MAX_POSITIONS_AS_BYTEARRAYS

	// DrawGridLayer::draw (uses 0x486a6000)
	// why is it split into three places?
	PATCH(0x372928 - ANDROID32_BASE, objPosPatch);
	PATCH(0x37304c - ANDROID32_BASE, objPosPatch);
	PATCH(0x373558 - ANDROID32_BASE, objPosPatch);

	// EditorUI::getLimitedPosition (uses 0x486a6000)
	PATCH(0x382bd8 - ANDROID32_BASE, objPosPatch);

	// EditorUI::onCreateObject (uses 0x486a6000)
	PATCH(0x3a5364 - ANDROID32_BASE, objPosPatch);

	// EditorUI::constrainGameLayerPosition (uses 0x486a6780)
	PATCH(0x382f7c - ANDROID32_BASE, cameraPosPatch);

	// EditorUI::getGroupCenter (uses 0x497423f0)
	PATCH(0x383474 - ANDROID32_BASE, maxCtrPosPatch);
	#elif defined(GEODE_IS_WINDOWS)
	// for Windows, you need to find 0x486a6780 [240030.0f] and 0x486a6000 [240000.0f]
	// addresses based on https://files.catbox.moe/joabt9.png [for 240030]
	// and https://files.catbox.moe/pjzqho.png [for 240000]
	// EditorUI::getGroupCenter UPDATE: also find 999999.f!
	// in Ghidra 12.0 and newer, just type the floats in manually.

	MAX_POSITIONS_AS_BYTEARRAYS

	/*
	EditorUI::onCreateObject,
	EditorUI::moveObject,
	EditorUI::getLimitedPosition,
	draw:1402dbbdd(R), draw:1402dc78b(R), draw:1402dc7d7(R), draw:1402dc9cd(R), draw:1402dcbcc(R), draw:1402dcc22(R)
	(all of them use 0x486a6000)
	*/
	PATCH(0x607ca0, objPosPatch);

	// EditorUI::constrainGameLayerPosition (uses 0x486a6780)
	PATCH(0x607ca4, cameraPosPatch);

	// EditorUI::getGroupCenter (uses 0x497423f0)
	PATCH(0x607cbc, maxCtrPosPatch);
	#elif defined(GEODE_IS_IOS)
	// Ery here.
	// I hate catering to JIT-less iOS.
	// Also, fuck Alan Dye for ruining any dignity that was left in macOS/iOS UI and UX.
	// Anyway.

	if (Loader::get()->isPatchless()) {
		// EditorUI::getLimitedPosition, EditorUI::onCreateObject, DrawGridLayer::draw (uses 0x486a6000)
		GEODE_MOD_STATIC_PATCH(0x63ccd8, { 0x28, 0x6B, 0x6E, 0x4E });

		// EditorUI::constrainGameLayerPosition (uses 0x486a6780)
		GEODE_MOD_STATIC_PATCH(0x63dbf0, { 0x29, 0x6B, 0x6E, 0x4E });

		// EditorUI::getGroupCenter (uses 0x497423f0)
		GEODE_MOD_STATIC_PATCH(0x63dbf8, { 0x28, 0x6B, 0x6E, 0x4E });
		return;
	}

	// Jasmine here again.
	// iOS, despite being ARM, is a bit different from macOS ARM
	// Instead of embedding the float into instructions, it is stored in data
	// and loaded into the instruction. This means we need to patch the data instead of the instruction.
	// Considering the nightmare that was the macOS ARM patching, this is a blessing.
	// We need to find 0x486a6780 [240030.0f] and 0x486a6000 [240000.0f].
	// EditorUI::getGroupCenter UPDATE: also find 999999.f!
	// in Ghidra 12.0 and newer, just type the floats in manually.

	MAX_POSITIONS_AS_BYTEARRAYS

	// EditorUI::getLimitedPosition, EditorUI::onCreateObject, DrawGridLayer::draw (uses 0x486a6000)
	PATCH(0x63ccd8, objPosPatch);

	// EditorUI::constrainGameLayerPosition (uses 0x486a6780)
	PATCH(0x63dbf0, cameraPosPatch);

	// EditorUI::getGroupCenter (uses 0x497423f0)
	PATCH(0x63dbf8, maxCtrPosPatch);

	// Alright, Jasmine out.
	#endif
}