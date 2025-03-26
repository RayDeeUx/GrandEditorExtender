#include <vector>
#include <cstdint>

using namespace geode::prelude;

#define PATCH(x, y) (void) Mod::get()->patch(reinterpret_cast<void*>(geode::base::get() + x), y);

// constexpr float maxObjPos = 240000.0f; // = 0x486a6000
// constexpr float maxCamPos = 240030.0f; // = 0x486a6780

// values determined by using qimiko's original 999'999'999.0f value
// and then improving them using https://www.h-schmidt.net/FloatConverter/IEEE754.html
// to avoid floating point precision loss
#define MAX_POSITIONS_AS_BYTEARRAYS\
	constexpr float maxObjPos = 1'000'000'000.0f;\
	constexpr float maxCamPos = 1'000'000'064.0f;\
	std::vector objPosPatch = floatToByteArray(maxObjPos);\
	std::vector cameraPosPatch = floatToByteArray(maxCamPos);\
	
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
	#elif defined(GEODE_IS_INTEL_MAC)
	// for macOS Intel, you need to find 0x486a6780 [240030.0f] and 0x486a6000 [240000.0f]
	// addresses based on https://files.catbox.moe/8su33t.png [for 240030]
	// and https://files.catbox.moe/rzpj6t.png [for 240000]

	MAX_POSITIONS_AS_BYTEARRAYS

	// max object position (used in too many functions to name)
	PATCH(0x7fdf4c, objPosPatch);

	// max camera position (only used in EditorUI::constrainGameLayerPosition)
	PATCH(0x7fdf34, cameraPosPatch);

	// a weird one
	PATCH(0x7fe110, objPosPatch);
	#elif defined(GEODE_IS_ANDROID64)
	// for Android 64-bit, you need to find 0x486a6780 [240030.0f] and 0x486a6000 [240000.0f]
	// addresses based on https://files.catbox.moe/3796mo.png [for 240030]
	// and https://files.catbox.moe/ujja7q.png [for 240000]

	MAX_POSITIONS_AS_BYTEARRAYS

	// DrawGridLayer::draw (uses 0x486a6000)
	PATCH(0x75d740 - ANDROID64_BASE, objPosPatch);

	// EditorUI::getLimitedPosition (uses 0x486a6000)
	PATCH(0x7773c4 - ANDROID64_BASE, objPosPatch);

	// EditorUI::onCreateObject (uses 0x486a6000)
	PATCH(0x7a7f44 - ANDROID64_BASE, objPosPatch);

	// EditorUI::constrainGameLayerPosition (uses 0x486a6780)
	PATCH(0x77790c - ANDROID64_BASE, cameraPosPatch);
	#elif defined(GEODE_IS_ANDROID32)
	// for Android 32-bit, you need to find 0x486a6780 [240030.0f] and 0x486a6000 [240000.0f]
	// addresses based on https://files.catbox.moe/bnztgs.png [for 240030]
	// and https://files.catbox.moe/tfgebw.png [for 240000]

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
	PATCH(0x607ca0, objPosPatch);

	// EditorUI::constrainGameLayerPosition (uses 0x486a6780)
	PATCH(0x607ca4, cameraPosPatch);
	#else
	#warning Unsupported platform for this mod.
	// I DO NOT GIVE CONSENT FOR THIS MOD TO BE PORTED TO IOS, FLERO CLIENT, OR SIMILAR.
	// ANY ATTEMPT TO CONTACT ME FOR PERMISSION TO PORT THIS MOD TO IOS WILL BE SWIFTLY IGNORED.
	// GO FIND THE ADDRESSES YOURSELVES, COWARDS.
	// -RayDeeUx
	geode::log::info("\n"
	"UNSUPPORTED PLATFORM DETECTED, LOSER!\n"
	"GRANDEDITOREXTENDER IS ILLEGITIMATE AND WILL NOT BE ALLOWED FOR USE ON IOS.\n"
	"PLEASE DISABLE THE MOD TO CONTINUE WITH YOUR DAY."
	"\n");
	/*




           SSNNNNNNNNNSS          NSSSNNSSSSSSN          SSNSSNNNSSSSSS                         SSNSNNNNSSS                                                                                                                                                                                                                                                                                        NNNNNNNNNN
        NNNSSXX     XXSSSSN    NSSSSXXXXXXXXSSSSSSNS  SNSSSXXXXXXXXXXSSSSSSSN               SNNSSXXXXXXXXSSSSSNS                                                                        SNNSSS                                                                                                                        SS                  SSSSSN                                                 NSSXXX    XSN
       NNX               XSN  SSX                XXNSSNSX                 SNN               NSX               XNNN   NNNNNNNNNNNNNNNNSS SNNNNNNNNNNNNNNNNNNSS NNNNNNNNNNNNNNNNN     SNNNSSSSSNNNSN    NNNNNNNNNNNNNNNNNNSS SSNNNNNNNNNNNNNNN   NNNNNNNNNNNNSS                               NNNNNNNNNN           NNNNSSSSSNNN          NNNSSSSSSNNNS     NNNNNNNNNNNNNNSS   SNNNNNNNNNNNNNNN     NX        XSN
     NNSX           XX     SNSNSX         XXXXX    XNSNS            XXXXXXSNSS              NSX           X     SSN NNXX           XXXSNSSX               XNSNSX            XXSSN NNSX         XXSNNSNSX               XSSNSSX            XXSNNSX         XXSSNN                           NSX      XSN      SSNSXX         XSNNN  NNSXX          XXSSNNSXX           XXXSNNSX            XXSNN  NX        XSS
    NNN           SNNNNX   XSNNS         XNNNNNSX   SNNS           NNNNNNNNSS               NSX         XNNNSX   XSNSS           XX   SNSX                XNNNX           XX  XSNNS               XSNNS                 SSNS           XX  XSNNX              XSNN                        NSX       XSN     SNS                SSSSSS                XSNS           XX  XSNNX           XX   XSN S        XSN
    NSS          XSNS NNX  XSNNS         XNNNNNS    NNNX           SNNSSS N                 NSX         XSSSNNX   SNNS         XSNNNNNNSNNNNNX        SNNNNSNNX        XSNNNNNNNNS         XNNNS   XSNSNNNSX       XNNNNNSNS         XNNNNNNNNNX        SNNSX  XSN                        NX        XSN     NSX     XSNNSX      SNNSX          XNNNSSNNNS         XNNSSNNSSNX        SNSNNSX  XNSNX       XSN
    NSX          SNN   NNNNNSSNS           XXX    XNNSNX            XXXXSN                  NSX         XSSSNSS   XNNS         XSNNNNSSS  SSNX       XSNSSSSSNX         XSSSNNNNNX        XSNSSNSXXSSNS  NSX       SNSS  SNS          SSSNNN SNX       XSNSNNX  XNN                       NX        XSN    NSS      XNSSNS      XSNNX           XSSNSSSNS         XSSNNNNNSNX        XSNNSS   XNSNX       SSS
    NSX          SNN   NSNSSNSNX                  XNNSNS                XNSS                NSX         SSSSNSS   XNNS              XNN    NNX       XNNS   SSX              SNSN         XSNS   NSSSS   NSX      XSNS    NS              XSSSNX       XSNS NX  XSN                       NXX       XSN    NSX     XSN  NSX     XSNNNX             XNNSNS              XNSSNX                XSNSNX       SN
    NSS          XSN  SNX  XSNNS         XSNSNNSX   SNNX           XSSSNNNSS                NSX         XSNSNNX   SNNS          XSSSSNSS   NNX       XSNS   SSX         XSSSSNNSN         XNN  JNSSNNN   NSX       SNS    NS          XSSSSNSSNX       XSNS NX  XSN                       NXX       XSN    NSX      XNS NSX     XSNSNSSSX            NNNS          SSSSNNSSNX              XSNNSSNSX      SN
     NNX          XNNNSX   XSNNS         XNNNSNNX   XNNS          XNSSSSSS                  NSX         XSNSSX   XSNNS         SNNSNSNSS   NNX       XNNS   SNX        XSNNSSSSNNX         SNNNNX  XSN   NSX       SNS    NS         XNNSSNSSSNX        SNSNS   XNNNSSSSN                 NSX       XSNNSSSNNS      XSNNNX      SSNNSSNNNS           SNNS         XNNSSNSSSNX        XX     XSNS  NSXXXXXSNSS
     SSNX                 XSNSNS          XSSSXX    SNNS          XNSS                      NSX                 XNNSSS          SNNNSSNN   NNX       XSNS   SNX          SNNNSSNNNX         XSXX   SNSS  NSX      XSNS    NS          XSNNSSNNNX         XX    XNNNX   XSN                NSX        XSSSSSSNSX       XXX      XSNNSX XSSX           SNNS          SNNNSSNNNX       XSNS      SN   NNNNNNNNN
       NNSX             XXSNSSNSX                 XSNNNSX         XNNS                      NSX              XSNNSSSSS                SNN  NNX       XSNS    NS               XSNSNSX            XSNSS   NSX      XSNS    NSX               SNNX             XSNNNS     SN                NSX              SNNNSX            XSNSSNNX              XSNSNS                SNNX       XSNSX     XSN NSX     XSN
         NNSSXXXXXXXXXSSNNSS   NSSSSXXXXXXXXXXSSSNNSSSSSSSXXXXXXXSNSSS                       NNSSSXXXXXXXSSSSNSSSSS NNSSSXXXXXXXXXXSSSNSS  NNNSXXXXXSNNSS    NNSSXXXXXXXXXXSSSSNS  NNSSXXXXXXXXSSNSSS     NSSXXXXXSNSS    SNSSSXXXXXXXXXXSSSNSNSSSXXXXXXXSSSNNSSSSNSX  XSNS                NSSXXXXXXXXXSSSSNSSNSNSSXXXXXXXXSSNSSSS NNNSSXXXXXXXXXSSNSSSNNSSSXXXXXXXXXSSSSNSNSSXXXXXSSNNNSXXXXXSNS NSXX   XSNSS
            SSSNNNNNNSSSSS        SSSSSSSSSSSSSS         SSSSSSSSSSS                            SSSSNSSNSSSSSS         SSSSNSSNSSSSSSSSS      SSSSNSSSSS        SSSSNSSSSSSSSSSS      SSSNNNNSSSSS          SSSSSSSS          SSSSSSSSSSSSS      SSSSSSSSSSSS      N  XSNS                    SSSSSSSSSSSSS        SSNNNNSSSSS          SNNNNNNSSSSS      SSSSSSSSSSSSSSS     SSSSSSS     SSSSSS    NNNNNNSSS
                                                                                                                                                                                                                                                                   NSSNSSS                                                                                                                           SSSSSSS


	*/
	#endif
}