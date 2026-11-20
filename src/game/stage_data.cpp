#include "Version.h"
#if defined(VERSION_R24J01)
#include "Dolphin/GX/GXFrameBuffer.h"
#include "game/wii/cgame.h"
#include "game/cfile.h"
extern "C" {
char* FormatToStaticBuffer(const char* format, ...);
s32 _prolog(s32 function, s32 count, s32* args);
}
// the game's own aligned array new: MEMAllocFromExpHeapEx on the MEM2 game heap
void* operator new[](size_t size, int align);
#else
#include "game/cgame.h"
#include "game/ut.h"
#include "Dolphin/GX/GXTypes.h"
#endif
#include "game/stage.h"
#include "game/globals.h"
#include <string.h>

// Stage offsets are measured from the start of the file.
struct StageFileHeader {
	u8 environment[4];
	u8 placements[4];
	u8 scriptOffsets[4];
	u8 setupScript[4];
	u8 entryScript[4];
	u8 batteryDepletedScript[4];
	u8 zButtonScript[4];
	u8 timerExpiredScript[4];
	u8 unknown20[4];
	u8 eventObjects[4];
	u8 cameraAreas[4];
	u8 cameraInfo[4];
	u8 cameraAreasEx[4];
};
STATIC_ASSERT(sizeof(StageFileHeader) == 0x34);

#if !defined(VERSION_R24J01)
extern "C" {
extern UnkStageObj gStageObjectPlacements[];
extern UnkObjData objTable[];
extern XObjSource gObjectSources[];
// HSD_VIInfo, its header needs the hsd include path
extern u8 HSD_VIData[];
BOOL LoadGameSoundBank(XSound* sound, s32 stage);
}

// retail's bss prefix ahead of gStageResources (0x140 on US/PAL, 0x134 on JP),
// used as the base register in __sinit_stage_data_cpp
#if defined(VERSION_PAL) || defined(VERSION_US)
u8 lbl_80367180[0x140];
#elif !defined(VERSION_R24J01)
u8 lbl_80345738[0x134];
#endif
Stage gStageResources;
Globals gGlobalScriptResources;

/**
 * @note Address: 0x8006EDB4
 * @note Size: 0x14
 */
extern "C" UnkStageObj* GetStageObjectPlacementByIndex(s32 index)
{
	return &gStageObjectPlacements[index];
}

extern "C" {

	// Resource path names, one object per name as in the retail .rodata
	const char sKitchenMapResourcePath[]               = "kitchen/map";
	const char sKitchenReizoukoResourcePath[]          = "kitchen/reizouko";
	const char sKitchenGasDaiResourcePath[]            = "kitchen/gas_dai";
	const char sKitchenSinkUnder1ResourcePath[]        = "kitchen/sink_under1";
	const char sKitchenSinkMidleResourcePath[]         = "kitchen/sink_midle";
	const char sKitchenTaoEResourcePath[]              = "kitchen/tao_e";
	const char sKitchenPlantBoxResourcePath[]          = "kitchen/plant_box";
	const char sKitchenCounter1ResourcePath[]          = "kitchen/counter1";
	const char sKitchenCounter2ResourcePath[]          = "kitchen/counter2";
	const char sKitchenTableResourcePath[]             = "kitchen/table";
	const char sKitchenShelfLResourcePath[]            = "kitchen/shelf_l";
	const char sKitchenDoorResourcePath[]              = "kitchen/door";
	const char sKitchenWallWalkResourcePath[]          = "kitchen/wall_walk";
	const char sNpcConsentmanResourcePath[]            = "npc/consentman";
	const char sNpcTonpyResourcePath[]                 = "npc/tonpy";
	const char sKitchenWindowSResourcePath[]           = "kitchen/window_s";
	const char sTestBox4ResourcePath[]                 = "test/box_4";
	const char sTestBox8ResourcePath[]                 = "test/box_8";
	const char sTestBox12ResourcePath[]                = "test/box_12";
	const char sTestBoxLiftResourcePath[]              = "test/box_lift";
	const char sTestRopeResourcePath[]                 = "test/rope";
	const char sTestLadderResourcePath[]               = "test/ladder";
	const char sRoukaMapResourcePath[]                 = "rouka/map";
	const char sRoukaDoorJResourcePath[]               = "rouka/door_j";
	const char sRoukaDoorKResourcePath[]               = "rouka/door_k";
	const char sRoukaDoorPamaResourcePath[]            = "rouka/door_pama";
	const char sRoukaEntranceResourcePath[]            = "rouka/entrance";
	const char sRoukaKaidan1ResourcePath[]             = "rouka/kaidan1";
	const char sRoukaKaidan2ResourcePath[]             = "rouka/kaidan2";
	const char sRoukaKaidan3ResourcePath[]             = "rouka/kaidan3";
	const char sRoukaMatResourcePath[]                 = "rouka/mat";
	const char sRoukaRoCounterResourcePath[]           = "rouka/ro_counter";
	const char sRoukaRoPlantResourcePath[]             = "rouka/ro_plant";
	const char sRoukaRoSofaResourcePath[]              = "rouka/ro_sofa";
	const char sRoukaTableKakuResourcePath[]           = "rouka/table_kaku";
	const char sRoukaTableMaruResourcePath[]           = "rouka/table_maru";
	const char sKitchenChair1ResourcePath[]            = "kitchen/chair1";
	const char sRoukaTesuri2fResourcePath[]            = "rouka/tesuri_2f";
	const char sRoukaWall12fResourcePath[]             = "rouka/wall_1_2f";
	const char sRoukaWall1fResourcePath[]              = "rouka/wall_1f";
	const char sRoukaWall2fResourcePath[]              = "rouka/wall_2f";
	const char sPushobj3ResourcePath[]                 = "pushobj3";
	const char sBackyardMapResourcePath[]              = "backyard/map";
	const char sBackyardBaseResourcePath[]             = "backyard/base";
	const char sBackyardBgResourcePath[]               = "backyard/bg";
	const char sBackyardDoorResourcePath[]             = "backyard/door";
	const char sBackyardFenceResourcePath[]            = "backyard/fence";
	const char sBackyardWallResourcePath[]             = "backyard/wall";
	const char sSoukoMapResourcePath[]                 = "souko/map";
	const char sSoukoWallResourcePath[]                = "souko/wall";
	const char sPushobj1ResourcePath[]                 = "pushobj1";
	const char sPushobj2ResourcePath[]                 = "pushobj2";
	const char sItemPenResourcePath[]                  = "item/pen";
	const char sKitchenWallResourcePath[]              = "kitchen/wall";
	const char sItemDekaDenchiResourcePath[]           = "item/deka_denchi";
	const char sChibihousePcGamenResourcePath[]        = "chibihouse/pc_gamen";
	const char sChibihouseGachaResourcePath[]          = "chibihouse/gacha";
	const char sChibihouseAirResourcePath[]            = "chibihouse/air";
	const char sJennyMapResourcePath[]                 = "jenny/map";
	const char sJennyBedResourcePath[]                 = "jenny/bed";
	const char sJennyChair1ResourcePath[]              = "jenny/chair1";
	const char sJennyChair2ResourcePath[]              = "jenny/chair2";
	const char sKitchenWindowResourcePath[]            = "kitchen/window";
	const char sJennyClosetResourcePath[]              = "jenny/closet";
	const char sJennyDeskResourcePath[]                = "jenny/desk";
	const char sJennyDoorResourcePath[]                = "jenny/door";
	const char sJennyDresserResourcePath[]             = "jenny/dresser";
	const char sJennyLight1ResourcePath[]              = "jenny/light1";
	const char sJennyMatResourcePath[]                 = "jenny/mat";
	const char sJennyShelfResourcePath[]               = "jenny/shelf";
	const char sJennyTableResourcePath[]               = "jenny/table";
	const char sJennyWallResourcePath[]                = "jenny/wall";
	const char sJennyWallShelfResourcePath[]           = "jenny/wall_shelf";
	const char sJennyWindow1ResourcePath[]             = "jenny/window1";
	const char sJennyWindow2ResourcePath[]             = "jenny/window2";
	const char sParentMapResourcePath[]                = "parent/map";
	const char sParentBarwallLResourcePath[]           = "parent/barwall_l";
	const char sParentBarwallMResourcePath[]           = "parent/barwall_m";
	const char sParentBarwallRResourcePath[]           = "parent/barwall_r";
	const char sParentBedResourcePath[]                = "parent/bed";
	const char sParentChairResourcePath[]              = "parent/chair";
	const char sParentClosetResourcePath[]             = "parent/closet";
	const char sParentCornershelfResourcePath[]        = "parent/cornershelf";
	const char sParentDresserResourcePath[]            = "parent/dresser";
	const char sParentSofaResourcePath[]               = "parent/sofa";
	const char sParentTableResourcePath[]              = "parent/table";
	const char sMapJumpBoxResourcePath[]               = "map_jump_box";
	const char sParentWallResourcePath[]               = "parent/wall";
	const char sParentWallshelfResourcePath[]          = "parent/wallshelf";
	const char sParentWindowResourcePath[]             = "parent/window";
	const char sNpcHockResourcePath[]                  = "npc/hock";
	const char sSoukoKaidanResourcePath[]              = "souko/kaidan";
	const char sSoukoDcDaiResourcePath[]               = "souko/dc_dai";
	const char sLivingShelfDanBoxResourcePath[]        = "living/shelf_dan_box";
	const char sLivingShelfDanDaiResourcePath[]        = "living/shelf_dan_dai";
	const char sLivingDoorResourcePath[]               = "living/door";
	const char sRoukaDoorLResourcePath[]               = "rouka/door_l";
	const char sRoukaDoorLDamiResourcePath[]           = "rouka/door_l_dami";
	const char sChibihouseCusCbPrLv1ResourcePath[]     = "chibihouse/cus_cb_pr_lv1";
	const char sChibihouseGachaPon1ResourcePath[]      = "chibihouse/gacha_pon1";
	const char sLivingWallShelf1ResourcePath[]         = "living/wall_shelf1";
	const char sLivingTelResourcePath[]                = "living/tel";
	const char sLivingTelCounterResourcePath[]         = "living/tel_counter";
	const char sLivingTelJResourcePath[]               = "living/tel_j";
	const char sLivingWallCounterResourcePath[]        = "living/wall_counter";
	const char sLivingShelfWResourcePath[]             = "living/shelf_w";
	const char sLivingWallUpResourcePath[]             = "living/wall_up";
	const char sLivingShelfSlopeResourcePath[]         = "living/shelf_slope";
	const char sLivingFlowerclockResourcePath[]        = "living/flowerclock";
	const char sSoukoHasiraResourcePath[]              = "souko/hasira";
	const char sLivingLivingSkyResourcePath[]          = "living/living_sky";
	const char sLivingButterflyResourcePath[]          = "living/butterfly";
	const char sLivingFlowerBedResourcePath[]          = "living/flower_bed";
	const char sRoukaKaidan2hResourcePath[]            = "rouka/kaidan_2h";
	const char sRoukaKaidan3hResourcePath[]            = "rouka/kaidan_3h";
	const char sSoukoWineBottleResourcePath[]          = "souko/wine_bottle";
	const char sRoukaHashiraTResourcePath[]            = "rouka/hashira_t";
	const char sRoukaHashiraEResourcePath[]            = "rouka/hashira_e";
	const char sRoukaWallUnderResourcePath[]           = "rouka/wall_under";
	const char sRoukaFloor2fResourcePath[]             = "rouka/floor_2f";
	const char sChibihouseMapResourcePath[]            = "chibihouse/map";
	const char sChibihouseWallResourcePath[]           = "chibihouse/wall";
	const char sChibihouseChiPcResourcePath[]          = "chibihouse/chi_pc";
	const char sChibihouseChiSocketResourcePath[]      = "chibihouse/chi_socket";
	const char sChibihouseChiEResourcePath[]           = "chibihouse/chi_e";
	const char sRoukaDoorEResourcePath[]               = "rouka/door_e";
	const char sRoukaWall2fUnderResourcePath[]         = "rouka/wall_2f_under";
	const char sChibiHouseResourcePath[]               = "chibi_house";
	const char sRoukaWindowResourcePath[]              = "rouka/window";
	const char sItemFrogRingResourcePath[]             = "Item/frog_ring";
	const char sChibiJumpResourcePath[]                = "chibi_jump";
	const char sNpcJennyResourcePath[]                 = "npc/jenny";
	const char sLivingMapResourcePath[]                = "living/map";
	const char sLivingDanroResourcePath[]              = "living/danro";
	const char sLivingShelfBResourcePath[]             = "living/shelf_b";
	const char sLivingShelfDanResourcePath[]           = "living/shelf_dan";
	const char sLivingSidetableResourcePath[]          = "living/sidetable";
	const char sLivingSofa1ResourcePath[]              = "living/sofa1";
	const char sLivingSofa2ResourcePath[]              = "living/sofa2";
	const char sLivingStandlightResourcePath[]         = "living/standlight";
	const char sLivingTableResourcePath[]              = "living/table";
	const char sLivingTvResourcePath[]                 = "living/tv";
	const char sLivingWallResourcePath[]               = "living/wall";
	const char sLivingWindowResourcePath[]             = "living/window";
	const char sHouseRChibiHMainResourcePath[]         = "house_r/chibi_h_main";
	const char sHouseRChibiHDaiResourcePath[]          = "house_r/chibi_h_dai";
	const char sNpcMamaResourcePath[]                  = "npc/mama";
	const char sLivingSofaHashigoResourcePath[]        = "living/sofa_hashigo";
	const char sLivingBooks1ResourcePath[]             = "living/books1";
	const char sLivingBooks2ResourcePath[]             = "living/books2";
	const char sLivingSofaCu1ResourcePath[]            = "living/sofa_cu1";
	const char sLivingSofaCu2ResourcePath[]            = "living/sofa_cu2";
	const char sLivingFlowerpot2ResourcePath[]         = "living/flowerpot2";
	const char sLivingShelfDanBookResourcePath[]       = "living/shelf_dan_book";
	const char sLivingTakoResourcePath[]               = "living/tako";
	const char sLivingOekakiResourcePath[]             = "living/oekaki";
	const char sHouseRChibiHHasigoResourcePath[]       = "house_r/chibi_h_hasigo";
	const char sNpcArmy1ResourcePath[]                 = "npc/army1";
	const char sNpcArmy2ResourcePath[]                 = "npc/army2";
	const char sHouseRChibiHDoor1ResourcePath[]        = "house_r/chibi_h_door1";
	const char sHouseRChibiHDoor2ResourcePath[]        = "house_r/chibi_h_door2";
	const char sDoorroomResourcePath[]                 = "doorroom";
	const char sDoorroomDokanResourcePath[]            = "doorroom_dokan";
	const char sItemSnackBoneResourcePath[]            = "item/snack_bone";
	const char sItemJunkUnkoResourcePath[]             = "item/junk_unko";
	const char sItemJunkAResourcePath[]                = "item/junk_a";
	const char sTestTestRingResourcePath[]             = "test/test_ring";
	const char sLivingTableFootResourcePath[]          = "living/table_foot";
	const char sLivingMatResourcePath[]                = "living/mat";
	const char sLivingTubResourcePath[]                = "living/tub";
	const char sPlugStartResourcePath[]                = "plug_start";
	const char sNpcDecaRoboResourcePath[]              = "npc/deca_robo";
	const char sNpcKeathResourcePath[]                 = "npc/keath";
	const char sChibihouseDenti1ResourcePath[]         = "chibihouse/denti1";
	const char sChibihouseDenti2ResourcePath[]         = "chibihouse/denti2";
	const char sNpcPapaResourcePath[]                  = "npc/papa";
	const char sHouseRChibiHHasiResourcePath[]         = "house_r/chibi_h_hasi";
	const char sHouseRChibiHWarpResourcePath[]         = "house_r/chibi_h_warp";
	const char sNpcFafaResourcePath[]                  = "npc/fafa";
	const char sNpcSpider1ResourcePath[]               = "npc/spider1";
	const char sItemBrushResourcePath[]                = "item/brush";
	const char sLivingSakanaResourcePath[]             = "living/sakana";
	const char sLivingKaigaJResourcePath[]             = "living/kaiga_j";
	const char sItemCapsuleResourcePath[]              = "item/capsule";
	const char sBackyardAriResourcePath[]              = "backyard/ari";
	const char sLivingBgResourcePath[]                 = "living/bg";
	const char sBackyardAriDaidaiResourcePath[]        = "backyard/ari_daidai";
	const char sBackyardTreeBResourcePath[]            = "backyard/tree_b";
	const char sBackyardKadanResourcePath[]            = "backyard/kadan";
	const char sPlugDokodakadoorResourcePath[]         = "plug_dokodakadoor";
	const char sLivingBbDanResourcePath[]              = "living/bb_dan";
	const char sLivingTrashboxResourcePath[]           = "living/trashbox";
	const char sLivingTaoDishResourcePath[]            = "living/tao_dish";
	const char sSoukoSpiderSuBResourcePath[]           = "souko/spider_su_b";
	const char sLivingSuisouR2ResourcePath[]           = "living/suisou_r2";
	const char sLivingSakanaAwaResourcePath[]          = "living/sakana_awa";
	const char sSoukoKumoHimoResourcePath[]            = "souko/kumo_himo";
	const char sItemKamiKuzuResourcePath[]             = "item/kami_kuzu";
	const char sItemCookieKakeraResourcePath[]         = "item/cookie_kakera";
	const char sLivingTvKeathResourcePath[]            = "living/tv_keath";
	const char sTestDust0ResourcePath[]                = "test/dust0";
	const char sTestDust1ResourcePath[]                = "test/dust1";
	const char sLivingSakanaEsaResourcePath[]          = "living/sakana_esa";
	const char sLivingTanaAnaResourcePath[]            = "living/tana_ana";
	const char sTestSpItoResourcePath[]                = "test/sp_ito";
	const char sLivingWallCornerResourcePath[]         = "living/wall_corner";
	const char sLivingCoUnderResourcePath[]            = "living/co_under";
	const char sTestJennyHandResourcePath[]            = "test/jenny_hand";
	const char sLivingWindowKazariResourcePath[]       = "living/window_kazari";
	const char sItemJunkBResourcePath[]                = "item/junk_b";
	const char sItemJunkCResourcePath[]                = "item/junk_c";
	const char sLivingGabyouResourcePath[]             = "living/gabyou";
	const char sBackyardSkyResourcePath[]              = "backyard/sky";
	const char sTestFlyRoot1ResourcePath[]             = "test/fly_root1";
	const char sNpcArmy3ResourcePath[]                 = "npc/army3";
	const char sNpcNWing1ResourcePath[]                = "npc/n_wing1";
	const char sNpcNWing2ResourcePath[]                = "npc/n_wing2";
	const char sRoukaPlanterResourcePath[]             = "rouka/planter";
	const char sItemSpoonResourcePath[]                = "item/spoon";
	const char sTestNMatoResourcePath[]                = "test/n_mato";
	const char sJennyCubeBoxResourcePath[]             = "jenny/cube_box";
	const char sJennyCubeLResourcePath[]               = "jenny/cube_l";
	const char sJennyCubeOResourcePath[]               = "jenny/cube_o";
	const char sJennyCubeVResourcePath[]               = "jenny/cube_v";
	const char sJennyCubeEResourcePath[]               = "jenny/cube_e";
	const char sJennyCuAnimal1ResourcePath[]           = "jenny/cu_animal1";
	const char sJennyCuAnimal2ResourcePath[]           = "jenny/cu_animal2";
	const char sItemMagCupResourcePath[]               = "item/mag_cup";
	const char sNpcSunMidaResourcePath[]               = "npc/sun_mida";
	const char sNpcSunDaiResourcePath[]                = "npc/sun_dai";
	const char sLivingCakeResourcePath[]               = "living/cake";
	const char sItemBaraResourcePath[]                 = "item/bara";
	const char sLivingOrangeGlassResourcePath[]        = "living/orange_glass";
	const char sLivingOrangeBinResourcePath[]          = "living/orange_bin";
	const char sLivingChamGlassResourcePath[]          = "living/cham_glass";
	const char sLivingChamBinResourcePath[]            = "living/cham_bin";
	const char sLivingVaseResourcePath[]               = "living/vase";
	const char sLivingChickenResourcePath[]            = "living/chicken";
	const char sLivingLivingCamResourcePath[]          = "living/living_cam";
	const char sNpcSpider2ResourcePath[]               = "npc/spider2";
	const char sLivingCakeDishResourcePath[]           = "living/cake_dish";
	const char sLivingKazariTResourcePath[]            = "living/kazari_t";
	const char sKitchenDoorRResourcePath[]             = "kitchen/door_r";
	const char sLivingAsiatoResourcePath[]             = "living/asiato";
	const char sLivingHatResourcePath[]                = "living/hat";
	const char sLivingKazariWResourcePath[]            = "living/kazari_w";
	const char sLivingBirthdayCResourcePath[]          = "living/birthday_c";
	const char sLivingBoneRResourcePath[]              = "living/bone_r";
	const char sKitchenDoorDamiResourcePath[]          = "kitchen/door_dami";
	const char sItemCosObakeResourcePath[]             = "item/cos_obake";
	const char sKitchenTutiHutaResourcePath[]          = "kitchen/tuti_huta";
	const char sKitchenSinkHiResourcePath[]            = "kitchen/sink_hi";
	const char sKitchenSinkHiUnderResourcePath[]       = "kitchen/sink_hi_under";
	const char sKitchenTrashboxResourcePath[]          = "kitchen/trashbox";
	const char sKitchenHammockResourcePath[]           = "kitchen/hammock";
	const char sRoukaShoesResourcePath[]               = "rouka/shoes";
	const char sKitchenSink2hiResourcePath[]           = "kitchen/sink_2hi";
	const char sRoukaSRopeResourcePath[]               = "rouka/s_rope";
	const char sLivingHappyBoxResourcePath[]           = "living/happy_box";
	const char sLivingConsentmanAtariResourcePath[]    = "living/consentman_atari";
	const char sItemTonpyItemAResourcePath[]           = "item/tonpy_item_a";
	const char sItemTonpyItemBResourcePath[]           = "item/tonpy_item_b";
	const char sItemTonpyItemCResourcePath[]           = "item/tonpy_item_c";
	const char sItemHockTizuResourcePath[]             = "item/hock_tizu";
	const char sHowToChibiroboCbBookResourcePath[]     = "how_to_chibirobo/cb_book";
	const char sFxCbHouse01ResourcePath[]              = "fx_cb_house01";
	const char sLivingCbCoverResourcePath[]            = "living/cb_cover";
	const char sLivingHappyHeartResourcePath[]         = "living/happy_heart";
	const char sAllYogoreCookOResourcePath[]           = "all/yogore_cook_o";
	const char sAllYogoreWaterResourcePath[]           = "all/yogore_water";
	const char sAllYogoreOilResourcePath[]             = "all/yogore_oil";
	const char sLivingCamLivingResourcePath[]          = "living/cam_living";
	const char sHouseRChibiHMakuResourcePath[]         = "house_r/chibi_h_maku";
	const char sKitchenYakanResourcePath[]             = "kitchen/yakan";
	const char sKitchenTeacupResourcePath[]            = "kitchen/teacup";
	const char sKitchenTeapotResourcePath[]            = "kitchen/teapot";
	const char sKitchenMilkBinResourcePath[]           = "kitchen/milk_bin";
	const char sKitchenSugarBinResourcePath[]          = "kitchen/sugar_bin";
	const char sKitchenCookieDishResourcePath[]        = "kitchen/cookie_dish";
	const char sKitchenCbTeasetResourcePath[]          = "kitchen/cb_teaset";
	const char sItemBlockSugarResourcePath[]           = "item/block_sugar";
	const char sItemFlowerCookieResourcePath[]         = "item/flower_cookie";
	const char sKitchenSinkSShelfResourcePath[]        = "kitchen/sink_s_shelf";
	const char sAllSpiderKageResourcePath[]            = "all/spider_kage";
	const char sKitchenSinkDishResourcePath[]          = "kitchen/sink_dish";
	const char sKitchenHukinResourcePath[]             = "kitchen/hukin";
	const char sKitchenSinkKagoResourcePath[]          = "kitchen/sink_kago";
	const char sLivingTvSwitchResourcePath[]           = "living/tv_switch";
	const char sKitchenTeacupSResourcePath[]           = "kitchen/teacup_s";
	const char sLivingCrackerResourcePath[]            = "living/cracker";
	const char sFxDecaSpark01ResourcePath[]            = "fx_deca_spark01";
	const char sLivingTvNewsResourcePath[]             = "living/tv_news";
	const char sLivingTvKageResourcePath[]             = "living/tv_kage";
	const char sLivingCbBook2ResourcePath[]            = "living/cb_book2";
	const char sFxTompCry01ResourcePath[]              = "fx_tomp_cry01";
	const char sLivingSw1planterResourcePath[]         = "living/sw_1planter";
	const char sLivingSw2planterResourcePath[]         = "living/sw_2planter";
	const char sKitchenBbNikkiResourcePath[]           = "kitchen/bb_nikki";
	const char sKitchenBbPenResourcePath[]             = "kitchen/bb_pen";
	const char sLivingWallUnderResourcePath[]          = "living/wall_under";
	const char sHowToChibiroboMapResourcePath[]        = "how_to_chibirobo/map";
	const char sFxTompScare01ResourcePath[]            = "fx_tomp_scare01";
	const char sLivingDoorKResourcePath[]              = "living/door_k";
	const char sItemCrackerResourcePath[]              = "item/cracker";
	const char sLivingCrackerGomiResourcePath[]        = "living/cracker_gomi";
	const char sLivingFClockHbResourcePath[]           = "living/f_clock_hb";
	const char sLivingFClockHsResourcePath[]           = "living/f_clock_hs";
	const char sLivingFClockFResourcePath[]            = "living/f_clock_f";
	const char sLivingPresentBoxResourcePath[]         = "living/present_box";
	const char sLivingBgTreeResourcePath[]             = "living/bg_tree";
	const char sFxSpider01ResourcePath[]               = "fx_spider01";
	const char sLivingChickenBoneResourcePath[]        = "living/chicken_bone";
	const char sLivingCakeAtoResourcePath[]            = "living/cake_ato";
	const char sRoukaCupArmyResourcePath[]             = "rouka/cup_army";
	const char sSoukoWineBottleAResourcePath[]         = "souko/wine_bottle_a";
	const char sSoukoWineBottleBResourcePath[]         = "souko/wine_bottle_b";
	const char sKitchenSink3hiResourcePath[]           = "kitchen/sink_3hi";
	const char sJennyTana1ResourcePath[]               = "jenny/tana1";
	const char sItemChipResourcePath[]                 = "item/chip";
	const char sLivingWindowWareResourcePath[]         = "living/window_ware";
	const char sLivingWindowHirakuResourcePath[]       = "living/window_hiraku";
	const char sNpcArmy4ResourcePath[]                 = "npc/army4";
	const char sNpcPrimoResourcePath[]                 = "npc/primo";
	const char sParentPartitionResourcePath[]          = "parent/partition";
	const char sParentPartition2ResourcePath[]         = "parent/partition2";
	const char sParentPartition3ResourcePath[]         = "parent/partition3";
	const char sParentPartition5ResourcePath[]         = "parent/partition5";
	const char sParentWallshelf2ResourcePath[]         = "parent/wallshelf2";
	const char sParentWallshelf3ResourcePath[]         = "parent/wallshelf3";
	const char sParentWallshelf4ResourcePath[]         = "parent/wallshelf4";
	const char sParentWallshelf5ResourcePath[]         = "parent/wallshelf5";
	const char sParentWallshelf6ResourcePath[]         = "parent/wallshelf6";
	const char sParentSwitchResourcePath[]             = "parent/switch";
	const char sHouseRDream1ResourcePath[]             = "house_r/dream1";
	const char sParentSidetableResourcePath[]          = "parent/sidetable";
	const char sParentLadderResourcePath[]             = "parent/ladder";
	const char sParentCloset2ResourcePath[]            = "parent/closet2";
	const char sParentDoorResourcePath[]               = "parent/door";
	const char sParentTable2ResourcePath[]             = "parent/table2";
	const char sParentChestResourcePath[]              = "parent/chest";
	const char sParentChest2ResourcePath[]             = "parent/chest2";
	const char sParentBlindRResourcePath[]             = "parent/blind_r";
	const char sParentBlindLResourcePath[]             = "parent/blind_l";
	const char sParentBlindhandleRResourcePath[]       = "parent/blindhandle_r";
	const char sParentBlindhandleLResourcePath[]       = "parent/blindhandle_l";
	const char sParentSewingmachineResourcePath[]      = "parent/sewingmachine";
	const char sJennyFlowerLampResourcePath[]          = "jenny/flower_lamp";
	const char sItemReceiptResourcePath[]              = "item/receipt";
	const char sParentMamaPenResourcePath[]            = "parent/mama_pen";
	const char sParentMamaCalculatorResourcePath[]     = "parent/mama_calculator";
	const char sParentBillsResourcePath[]              = "parent/bills";
	const char sParentReceiptResourcePath[]            = "parent/receipt";
	const char sParentAccountBookResourcePath[]        = "parent/account_book";
	const char sUfoWallResourcePath[]                  = "ufo/wall";
	const char sItemTyuusyakiResourcePath[]            = "item/tyuusyaki";
	const char sKitchenHamburgResourcePath[]           = "kitchen/hamburg";
	const char sJennyJennyTvResourcePath[]             = "jenny/jenny_tv";
	const char sNpcFunkyFResourcePath[]                = "npc/funky_f";
	const char sNpcTRexResourcePath[]                  = "npc/t_rex";
	const char sAllUfoWarpResourcePath[]               = "all/ufo_warp";
	const char sAllCllPapaResourcePath[]               = "all/cll_papa";
	const char sAllCllMamaResourcePath[]               = "all/cll_mama";
	const char sKitchenGasPlateResourcePath[]          = "kitchen/gas_plate";
	const char sParentDoorbellResourcePath[]           = "parent/doorbell";
	const char sParentDoorwindowResourcePath[]         = "parent/doorwindow";
	const char sParentDoorwindow2ResourcePath[]        = "parent/doorwindow2";
	const char sParentPlanterResourcePath[]            = "parent/planter";
	const char sParentPlanter2ResourcePath[]           = "parent/planter2";
	const char sParentPoleResourcePath[]               = "parent/pole";
	const char sParentPole2ResourcePath[]              = "parent/pole2";
	const char sParentPole3ResourcePath[]              = "parent/pole3";
	const char sParentPole4ResourcePath[]              = "parent/pole4";
	const char sParentPole5ResourcePath[]              = "parent/pole5";
	const char sParentPole6ResourcePath[]              = "parent/pole6";
	const char sParentPole7ResourcePath[]              = "parent/pole7";
	const char sParentPole8ResourcePath[]              = "parent/pole8";
	const char sParentRecplayerResourcePath[]          = "parent/recplayer";
	const char sParentRecspeakerLResourcePath[]        = "parent/recspeaker_l";
	const char sParentRecspeakerRResourcePath[]        = "parent/recspeaker_r";
	const char sParentStpoleResourcePath[]             = "parent/stpole";
	const char sParentStpole2ResourcePath[]            = "parent/stpole2";
	const char sItemTonpyItemEResourcePath[]           = "item/tonpy_item_e";
	const char sItemTonpyItemFResourcePath[]           = "item/tonpy_item_f";
	const char sItemTonpyItemGResourcePath[]           = "item/tonpy_item_g";
	const char sItemTonpyItemHResourcePath[]           = "item/tonpy_item_h";
	const char sItemTonpyItemIResourcePath[]           = "item/tonpy_item_i";
	const char sItemTonpyItemJResourcePath[]           = "item/tonpy_item_j";
	const char sItemTonpyItemKResourcePath[]           = "item/tonpy_item_k";
	const char sItemTonpyItemLResourcePath[]           = "item/tonpy_item_l";
	const char sItemTonpyItemMResourcePath[]           = "item/tonpy_item_m";
	const char sItemTonpyItemNResourcePath[]           = "item/tonpy_item_n";
	const char sItemTonpyItemOResourcePath[]           = "item/tonpy_item_o";
	const char sItemTonpyItemPResourcePath[]           = "item/tonpy_item_p";
	const char sItemTonpyItemQResourcePath[]           = "item/tonpy_item_q";
	const char sItemTonpyItemRResourcePath[]           = "item/tonpy_item_r";
	const char sItemTonpyItemSResourcePath[]           = "item/tonpy_item_s";
	const char sItemTonpyItemTResourcePath[]           = "item/tonpy_item_t";
	const char sItemTonpyItemUResourcePath[]           = "item/tonpy_item_u";
	const char sBackyardWindow1lResourcePath[]         = "backyard/window_1l";
	const char sBackyardWindow2lResourcePath[]         = "backyard/window_2l";
	const char sItemLetterResourcePath[]               = "item/letter";
	const char sParentDoorswitchResourcePath[]         = "parent/doorswitch";
	const char sJennyAsiba1ResourcePath[]              = "jenny/asiba1";
	const char sJennyAsiba2ResourcePath[]              = "jenny/asiba2";
	const char sJennyAsiba3ResourcePath[]              = "jenny/asiba3";
	const char sJennyCastleDoorResourcePath[]          = "jenny/castle_door";
	const char sJennyHeitai1ResourcePath[]             = "jenny/heitai1";
	const char sJennyKaidanResourcePath[]              = "jenny/kaidan";
	const char sJennyHikidasiCloseResourcePath[]       = "jenny/hikidasi_close";
	const char sJennyHikidasiOpenResourcePath[]        = "jenny/hikidasi_open";
	const char sRoukaDoorPamaHResourcePath[]           = "rouka/door_pama_h";
	const char sRoukaRopeDoorResourcePath[]            = "rouka/rope_door";
	const char sRoukaRopeNobuResourcePath[]            = "rouka/rope_nobu";
	const char sJennyAsibaS1ResourcePath[]             = "jenny/asiba_s1";
	const char sJennyCastleHasigoResourcePath[]        = "jenny/castle_hasigo";
	const char sJennyHariLResourcePath[]               = "jenny/hari_l";
	const char sJennyHariSResourcePath[]               = "jenny/hari_s";
	const char sJennyHeitai2ResourcePath[]             = "jenny/heitai2";
	const char sJennyHeitai3ResourcePath[]             = "jenny/heitai3";
	const char sJennyHeitai4ResourcePath[]             = "jenny/heitai4";
	const char sItemDenchi1ResourcePath[]              = "item/denchi1";
	const char sItemDenchi2ResourcePath[]              = "item/denchi2";
	const char sItemDenchi3ResourcePath[]              = "item/denchi3";
	const char sItemPeetsKutuResourcePath[]            = "item/peets_kutu";
	const char sItemHocyoukiResourcePath[]             = "item/hocyouki";
	const char sJennyAsibaKado1ResourcePath[]          = "jenny/asiba_kado1";
	const char sJennyAsiba4ResourcePath[]              = "jenny/asiba4";
	const char sLivingMituFlowerResourcePath[]         = "living/mitu_flower";
	const char sJennyHariL2ResourcePath[]              = "jenny/hari_l2";
	const char sNpcPapaSleepResourcePath[]             = "npc/papa_sleep";
	const char sNpcMamaSleepResourcePath[]             = "npc/mama_sleep";
	const char sNpcJennySleepResourcePath[]            = "npc/jenny_sleep";
	const char sAllSaSealResourcePath[]                = "all/sa_seal";
	const char sParentSkirtingBoardResourcePath[]      = "parent/skirting_board";
	const char sParentSkirtingBoard2ResourcePath[]     = "parent/skirting_board2";
	const char sParentSkirtingBoardpResourcePath[]     = "parent/skirting_boardp";
	const char sParentRecplayerCableResourcePath[]     = "parent/recplayer_cable";
	const char sParentBed2ResourcePath[]               = "parent/bed2";
	const char sRoukaTakiObjResourcePath[]             = "rouka/taki_obj";
	const char sRoukaCeilingResourcePath[]             = "rouka/ceiling";
	const char sJennyTvGamenResourcePath[]             = "jenny/tv_gamen";
	const char sParentBedStayResourcePath[]            = "parent/bed_stay";
	const char sAllYogoreTaiyaResourcePath[]           = "all/yogore_taiya";
	const char sAllYogoreKureyonResourcePath[]         = "all/yogore_kureyon";
	const char sAllYogoreToyResourcePath[]             = "all/yogore_toy";
	const char sNpcFrogResourcePath[]                  = "npc/frog";
	const char sParentCornershelfLadderResourcePath[]  = "parent/cornershelf_ladder";
	const char sBackyardWindowWareResourcePath[]       = "backyard/window_ware";
	const char sBackyardJagutiResourcePath[]           = "backyard/jaguti";
	const char sJennyHasiResourcePath[]                = "jenny/hasi";
	const char sKitchenHamPanUpResourcePath[]          = "kitchen/ham_pan_up";
	const char sKitchenHamPanDownResourcePath[]        = "kitchen/ham_pan_down";
	const char sJennyHeitai5ResourcePath[]             = "jenny/heitai5";
	const char sFxLeticwarp01ResourcePath[]            = "fx_leticwarp01";
	const char sBackyardHoseResourcePath[]             = "backyard/hose";
	const char sRoukaNebukuroResourcePath[]            = "rouka/nebukuro";
	const char sParentCamPapamamaResourcePath[]        = "parent/cam_papamama";
	const char sJennyCamJennyResourcePath[]            = "jenny/cam_jenny";
	const char sAllEnjoyLightResourcePath[]            = "all/enjoy_light";
	const char sNpcZobinResourcePath[]                 = "npc/zobin";
	const char sNpcPeetsResourcePath[]                 = "npc/peets";
	const char sJennyHeitaiTaikoResourcePath[]         = "jenny/heitai_taiko";
	const char sJennyHeitaiRappaResourcePath[]         = "jenny/heitai_rappa";
	const char sJennyJennyHimoResourcePath[]           = "jenny/jenny_himo";
	const char sJennyJennyBagResourcePath[]            = "jenny/jenny_bag";
	const char sFxZactGiccoResourcePath[]              = "fx_zact_gicco";
	const char sFxZactTaoResourcePath[]                = "fx_zact_tao";
	const char sFxZactFrogResourcePath[]               = "fx_zact_frog";
	const char sFxZactInjuredResourcePath[]            = "fx_zact_injured";
	const char sFxZactPajamasResourcePath[]            = "fx_zact_pajamas";
	const char sParentSaisunBoxResourcePath[]          = "parent/saisun_box";
	const char sBackyardKumoUfoResourcePath[]          = "backyard/kumo_ufo";
	const char sLivingHappyHeart2ResourcePath[]        = "living/happy_heart2";
	const char sJennyBedTopResourcePath[]              = "jenny/bed_top";
	const char sAll1dayTimerResourcePath[]             = "all/1day_timer";
	const char sRoukaDoorStopResourcePath[]            = "rouka/door_stop";
	const char sItemRexToothResourcePath[]             = "item/rex_tooth";
	const char sJennyZobinBoxResourcePath[]            = "jenny/zobin_box";
	const char sRoukaKaidanAtamaResourcePath[]         = "rouka/kaidan_atama";
	const char sAllLeafResourcePath[]                  = "all/leaf";
	const char sRoukaDoorKazariResourcePath[]          = "rouka/door_kazari";
	const char sJennyHeitaiGuitarResourcePath[]        = "jenny/heitai_guitar";
	const char sNpcZobinBm1ResourcePath[]              = "npc/zobin_bm1";
	const char sItemTaneAResourcePath[]                = "item/tane_a";
	const char sItemBbPenResourcePath[]                = "item/bb_pen";
	const char sItemInkResourcePath[]                  = "item/ink";
	const char sItemKareFlowerResourcePath[]           = "item/kare_flower";
	const char sJennyPresentHanaResourcePath[]         = "jenny/present_hana";
	const char sLivingSunmidaskyResourcePath[]         = "living/sunmidasky";
	const char sItemKayakuResourcePath[]               = "item/kayaku";
	const char sItemGoggleResourcePath[]               = "item/goggle";
	const char sItemLoveLetterResourcePath[]           = "item/love_letter";
	const char sItemTugResourcePath[]                  = "item/tug";
	const char sItemTicketResourcePath[]               = "item/ticket";
	const char sItemHoutaiResourcePath[]               = "item/houtai";
	const char sItemBlockResourcePath[]                = "item/block";
	const char sItemRecordGiccioResourcePath[]         = "item/record_giccio";
	const char sItemRecordFfResourcePath[]             = "item/record_ff";
	const char sLivingWallCHResourcePath[]             = "living/wall_c_h";
	const char sItemNwingItemResourcePath[]            = "item/nwing_item";
	const char sItemArmyPhotoResourcePath[]            = "item/army_photo";
	const char sRoukaCounterHResourcePath[]            = "rouka/counter_h";
	const char sTestFlyRoot2ResourcePath[]             = "test/fly_root2";
	const char sNpcCar1ResourcePath[]                  = "npc/car1";
	const char sNpcCar2ResourcePath[]                  = "npc/car2";
	const char sNpcFunkyF2ResourcePath[]               = "npc/funky_f2";
	const char sSoukoShelfResourcePath[]               = "souko/shelf";
	const char sSoukoChestResourcePath[]               = "souko/chest";
	const char sSoukoRengaResourcePath[]               = "souko/renga";
	const char sKitchenKamareBbResourcePath[]          = "kitchen/kamare_bb";
	const char sSoukoShelfDamiResourcePath[]           = "souko/shelf_dami";
	const char sKitchenTutiAnaResourcePath[]           = "kitchen/tuti_ana";
	const char sNpcHockShipResourcePath[]              = "npc/hock_ship";
	const char sItemLeftFootResourcePath[]             = "item/left_foot";
	const char sParentSuitCaseResourcePath[]           = "parent/suit_case";
	const char sItemKairozuResourcePath[]              = "item/kairozu";
	const char sItemDekaDenchiFullResourcePath[]       = "item/deka_denchi_full";
	const char sJennyHagurumaResourcePath[]            = "jenny/haguruma";
	const char sJennyPlanterZobinResourcePath[]        = "jenny/planter_zobin";
	const char sJennyPlanterJenny1ResourcePath[]       = "jenny/planter_jenny1";
	const char sJennyPlanterJenny2ResourcePath[]       = "jenny/planter_jenny2";
	const char sUfoUfoBallResourcePath[]               = "ufo/ufo_ball";
	const char sUfoTimeMcResourcePath[]                = "ufo/time_mc";
	const char sItemPajamaKijiResourcePath[]           = "item/pajama_kiji";
	const char sJennyPresentHanaZobinResourcePath[]    = "jenny/present_hana_zobin";
	const char sAllFlower1ResourcePath[]               = "all/flower1";
	const char sAllFlower2ResourcePath[]               = "all/flower2";
	const char sAllFlower3ResourcePath[]               = "all/flower3";
	const char sAllJunkTreeResourcePath[]              = "all/junk_tree";
	const char sAllCoinTreeResourcePath[]              = "all/coin_tree";
	const char sAllHappyTreeResourcePath[]             = "all/happy_tree";
	const char sAllMameTreeResourcePath[]              = "all/mame_tree";
	const char sJennySanpuHouseResourcePath[]          = "jenny/sanpu_house";
	const char sItemKareFlowerMituResourcePath[]       = "item/kare_flower_mitu";
	const char sItemSakiFlower1ResourcePath[]          = "item/saki_flower1";
	const char sKitchenJaguchiHResourcePath[]          = "kitchen/jaguchi_h";
	const char sKitchenTutiHutaDResourcePath[]         = "kitchen/tuti_huta_d";
	const char sNpcTTrackResourcePath[]                = "npc/t_track";
	const char sNpcArmy5ResourcePath[]                 = "npc/army5";
	const char sUfoWall2ResourcePath[]                 = "ufo/wall2";
	const char sKitchenWallTonneruResourcePath[]       = "kitchen/wall_tonneru";
	const char sKitchenWallTWareResourcePath[]         = "kitchen/wall_t_ware";
	const char sJennyToyDenwaResourcePath[]            = "jenny/toy_denwa";
	const char sJennyToyPianoResourcePath[]            = "jenny/toy_piano";
	const char sUfoExitResourcePath[]                  = "ufo/exit";
	const char sLivingDensetuHanaResourcePath[]        = "living/densetu_hana";
	const char sHaisuiMapResourcePath[]                = "haisui/map";
	const char sHaisuiPipeAResourcePath[]              = "haisui/pipe_a";
	const char sHaisuiHashigoBResourcePath[]           = "haisui/hashigo_b";
	const char sHaisuiHashigoCResourcePath[]           = "haisui/hashigo_c";
	const char sHaisuiHashigoDResourcePath[]           = "haisui/hashigo_d";
	const char sUfoGroundResourcePath[]                = "ufo/ground";
	const char sUfoGround2ResourcePath[]               = "ufo/ground2";
	const char sUfoGround3ResourcePath[]               = "ufo/ground3";
	const char sUfoBallRotResourcePath[]               = "ufo/ball_rot";
	const char sUfoGame1ResourcePath[]                 = "ufo/game1";
	const char sUfoGame2ResourcePath[]                 = "ufo/game2";
	const char sUfoGame3ResourcePath[]                 = "ufo/game3";
	const char sUfoGame4ResourcePath[]                 = "ufo/game4";
	const char sUfoGameBotanResourcePath[]             = "ufo/game_botan";
	const char sUfoGameP1ResourcePath[]                = "ufo/game_p1";
	const char sUfoGameP2ResourcePath[]                = "ufo/game_p2";
	const char sUfoGameP3ResourcePath[]                = "ufo/game_p3";
	const char sUfoGameP4ResourcePath[]                = "ufo/game_p4";
	const char sUfoWall3ResourcePath[]                 = "ufo/wall3";
	const char sTestCarRoot1ResourcePath[]             = "test/car_root1";
	const char sTestCarRoot2ResourcePath[]             = "test/car_root2";
	const char sUfoGameBotan2ResourcePath[]            = "ufo/game_botan2";
	const char sTestNMato2ResourcePath[]               = "test/n_mato2";
	const char sKitchenSinkHashigoResourcePath[]       = "kitchen/sink_hashigo";
	const char sRoukaTutaAResourcePath[]               = "rouka/tuta_a";
	const char sRoukaTutaARaResourcePath[]             = "rouka/tuta_aRa";
	const char sRoukaTutaARbResourcePath[]             = "rouka/tuta_aRb";
	const char sJennyChibiIsuResourcePath[]            = "jenny/chibi_isu";
	const char sRoukaTutaBResourcePath[]               = "rouka/tuta_b";
	const char sRoukaTutaBRResourcePath[]              = "rouka/tuta_bR";
	const char sRoukaTutaCResourcePath[]               = "rouka/tuta_c";
	const char sRoukaTutaCRResourcePath[]              = "rouka/tuta_cR";
	const char sRoukaTutaDResourcePath[]               = "rouka/tuta_d";
	const char sRoukaTutaDRResourcePath[]              = "rouka/tuta_dR";
	const char sKitchenConroResourcePath[]             = "kitchen/conro";
	const char sKitchenKingyoResourcePath[]            = "kitchen/kingyo";
	const char sKitchenWPlanterResourcePath[]          = "kitchen/w_planter";
	const char sRoukaTakiHashiAResourcePath[]          = "rouka/taki_hashi_a";
	const char sRoukaTakiHashiBResourcePath[]          = "rouka/taki_hashi_b";
	const char sJennyKenban1ResourcePath[]             = "jenny/kenban1";
	const char sJennyKenban2ResourcePath[]             = "jenny/kenban2";
	const char sParentTpMike1ResourcePath[]            = "parent/tp_mike1";
	const char sParentTpMike2ResourcePath[]            = "parent/tp_mike2";
	const char sJennyHouseHimoResourcePath[]           = "jenny/house_himo";
	const char sJennyBagHimoResourcePath[]             = "jenny/bag_himo";
	const char sRoukaTakiKakoiResourcePath[]           = "rouka/taki_kakoi";
	const char sRoukaArmyIta1ResourcePath[]            = "rouka/army_ita1";
	const char sRoukaArmyIta2ResourcePath[]            = "rouka/army_ita2";
	const char sRoukaArmyHeri1ResourcePath[]           = "rouka/army_heri1";
	const char sUfoUfoBedResourcePath[]                = "ufo/ufo_bed";
	const char sJennyJennyDressResourcePath[]          = "jenny/jenny_dress";
	const char sFxKitchenburnerResourcePath[]          = "fx_kitchenburner";
	const char sFxWarpzoneResourcePath[]               = "fx_warpzone";
	const char sUfoUfoBed2ResourcePath[]               = "ufo/ufo_bed2";
	const char sUfoUfoBed3ResourcePath[]               = "ufo/ufo_bed3";
	const char sUfoUfoBed4ResourcePath[]               = "ufo/ufo_bed4";
	const char sNpcNasubiResourcePath[]                = "npc/nasubi";
	const char sAllTonpyItemKageResourcePath[]         = "all/tonpy_item_kage";
	const char sRoukaRaceItaResourcePath[]             = "rouka/race_ita";
	const char sHaisuiSotoResourcePath[]               = "haisui/soto";
	const char sHaisuiNezumiResourcePath[]             = "haisui/nezumi";
	const char sHaisuiHashigoFResourcePath[]           = "haisui/hashigo_f";
	const char sHaisuiRopeResourcePath[]               = "haisui/rope";
	const char sHaisuiPipeBResourcePath[]              = "haisui/pipe_b";
	const char sJennyJennyDress2ResourcePath[]         = "jenny/jenny_dress2";
	const char sJennyJennyDress3ResourcePath[]         = "jenny/jenny_dress3";
	const char sHaisuiKabeResourcePath[]               = "haisui/kabe";
	const char sSoukoBirdcageResourcePath[]            = "souko/birdcage";
	const char sSoukoBoardResourcePath[]               = "souko/board";
	const char sSoukoNailResourcePath[]                = "souko/nail";
	const char sSoukoVRopeResourcePath[]               = "souko/v_rope";
	const char sHaisuiWaterResourcePath[]              = "haisui/water";
	const char sItemPajamaKiji2ResourcePath[]          = "item/pajama_kiji2";
	const char sItemPajamaKiji3ResourcePath[]          = "item/pajama_kiji3";
	const char sParentTKazan1ResourcePath[]            = "parent/t_kazan1";
	const char sParentTKazan2ResourcePath[]            = "parent/t_kazan2";
	const char sParentTKazan3ResourcePath[]            = "parent/t_kazan3";
	const char sParentTKazan4ResourcePath[]            = "parent/t_kazan4";
	const char sParentTKazan5ResourcePath[]            = "parent/t_kazan5";
	const char sParentTKazan6ResourcePath[]            = "parent/t_kazan6";
	const char sItemTBlock1ResourcePath[]              = "item/t_block1";
	const char sItemTBlock2ResourcePath[]              = "item/t_block2";
	const char sItemTBlock3ResourcePath[]              = "item/t_block3";
	const char sItemTBlock4ResourcePath[]              = "item/t_block4";
	const char sItemTBlock5ResourcePath[]              = "item/t_block5";
	const char sItemTBlock6ResourcePath[]              = "item/t_block6";
	const char sItemCarItemResourcePath[]              = "item/car_item";
	const char sBackyardTHashigoResourcePath[]         = "backyard/t_hashigo";
	const char sBackyardBrankoResourcePath[]           = "backyard/branko";
	const char sBackyardBRopeResourcePath[]            = "backyard/b_rope";
	const char sItemPapaYubiwaResourcePath[]           = "item/papa_yubiwa";
	const char sParentTKageResourcePath[]              = "parent/t_kage";
	const char sFxZactGhostResourcePath[]              = "fx_zact_ghost";
	const char sParentFrogHagataResourcePath[]         = "parent/frog_hagata";
	const char sItemKusaResourcePath[]                 = "item/kusa";
	const char sRoukaBebeBinsenResourcePath[]          = "rouka/bebe_binsen";
	const char sItemSekkeizuResourcePath[]             = "item/sekkeizu";
	const char sItemItemFrogResourcePath[]             = "item/item_frog";
	const char sFxPcBuy01ResourcePath[]                = "fx_pc_buy01";
	const char sJennyJennyBookResourcePath[]           = "jenny/jenny_book";
	const char sJennyPlanterHachiResourcePath[]        = "jenny/planter_hachi";
	const char sJennyKureyonResourcePath[]             = "jenny/kureyon";
	const char sFxCbdoor01ResourcePath[]               = "fx_cbdoor01";
	const char sFxTmBarrierResourcePath[]              = "fx_tm_barrier";
	const char sJennySuberidai1ResourcePath[]          = "jenny/suberidai1";
	const char sJennySuberidai2ResourcePath[]          = "jenny/suberidai2";
	const char sLivingAlbumTResourcePath[]             = "living/album_t";
	const char sLivingAlbumYResourcePath[]             = "living/album_y";
	const char sLivingTableStandResourcePath[]         = "living/table_stand";
	const char sLivingKaizouPoperResourcePath[]        = "living/kaizou_poper";
	const char sParentWeldingMaskpResourcePath[]       = "parent/welding_maskp";
	const char sParentWeldingMaskmResourcePath[]       = "parent/welding_maskm";
	const char sParentWeldingMaskjResourcePath[]       = "parent/welding_maskj";
	const char sParentWeldingMasktResourcePath[]       = "parent/welding_maskt";
	const char sParentWeldingTorchResourcePath[]       = "parent/welding_torch";
	const char sParentWeldingToolboxResourcePath[]     = "parent/welding_toolbox";
	const char sParentWeldingToolsResourcePath[]       = "parent/welding_tools";
	const char sParentDrafterResourcePath[]            = "parent/drafter";
	const char sKitchenGasHashigoResourcePath[]        = "kitchen/gas_hashigo";
	const char sKitchenGasRopeResourcePath[]           = "kitchen/gas_rope";
	const char sParentCaseJanksResourcePath[]          = "parent/case_janks";
	const char sYaneTorinosuResourcePath[]             = "yane/torinosu";
	const char sBackyardKusaTutiResourcePath[]         = "backyard/kusa_tuti";
	const char sLivingKeathOmcResourcePath[]           = "living/keath_omc";
	const char sTitleIconIconBbResourcePath[]          = "title_icon/icon_bb";
	const char sRoukaTaoBagResourcePath[]              = "rouka/tao_bag";
	const char sRoukaKokubanResourcePath[]             = "rouka/kokuban";
	const char sParentDcfootSpiderResourcePath[]       = "parent/dcfoot_spider";
	const char sAllTakarabakoResourcePath[]            = "all/takarabako";
	const char sCbCannonLv2ResourcePath[]              = "cb_cannon_lv2";
	const char sCbRadarResourcePath[]                  = "cb_radar";
	const char sLivingCbKaiPoperResourcePath[]         = "living/cb_kai_poper";
	const char sHaisuiRoom1CResourcePath[]             = "haisui/room1_c";
	const char sFxDecaEnergyResourcePath[]             = "fx_deca_energy";
	const char sFxDecaBatteryResourcePath[]            = "fx_deca_battery";
	const char sFx3pu01ResourcePath[]                  = "fx_3pu01";
	const char sFxPapa01ResourcePath[]                 = "fx_papa01";
	const char sKitchenKankisenResourcePath[]          = "kitchen/kankisen";
	const char sItemTamagottiResourcePath[]            = "item/tamagotti";
	const char sNpcDecaRobo2ResourcePath[]             = "npc/deca_robo2";
	const char sKitchenToasterResourcePath[]           = "kitchen/toaster";
	const char sKitchenSenzaiResourcePath[]            = "kitchen/senzai";
	const char sKitchenSuponjiResourcePath[]           = "kitchen/suponji";
	const char sKitchenJamResourcePath[]               = "kitchen/jam";
	const char sKitchenGusUpResourcePath[]             = "kitchen/gus_up";
	const char sAllYogoreHSResourcePath[]              = "all/yogore_h_s";
	const char sAllYogoreHKResourcePath[]              = "all/yogore_h_k";
	const char sNpcSpider3ResourcePath[]               = "npc/spider3";
	const char sNpcSunKaoResourcePath[]                = "npc/sun_kao";
	const char sKitchenPuroperaResourcePath[]          = "kitchen/puropera";
	const char sJennyCastleHasigo2ResourcePath[]       = "jenny/castle_hasigo2";
	const char sAllYogoreHitoResourcePath[]            = "all/yogore_hito";
	const char sAllYogoreHunResourcePath[]             = "all/yogore_hun";
	const char sAllYogoreShimiResourcePath[]           = "all/yogore_shimi";
	const char sFxArmySgtResourcePath[]                = "fx_army_sgt";
	const char sJennyJennyKagamiResourcePath[]         = "jenny/jenny_kagami";
	const char sLivingTamagottiGamenResourcePath[]     = "living/tamagotti_gamen";
	const char sKitchenKankisenAsibaResourcePath[]     = "kitchen/kankisen_asiba";
	const char sLivingLightRopeResourcePath[]          = "living/light_rope";
	const char sLivingAudioResourcePath[]              = "living/audio";
	const char sLivingSibariMamaResourcePath[]         = "living/sibari_mama";
	const char sLivingSibariPapaResourcePath[]         = "living/sibari_papa";
	const char sLivingSibariTaoResourcePath[]          = "living/sibari_tao";
	const char sLivingSibariJennyResourcePath[]        = "living/sibari_jenny";
	const char sItemAkikanResourcePath[]               = "item/akikan";
	const char sItemCandyGomiResourcePath[]            = "item/candy_gomi";
	const char sItemOkasiGomi1ResourcePath[]           = "item/okasi_gomi1";
	const char sItemOkasiGomi2ResourcePath[]           = "item/okasi_gomi2";
	const char sItemItemNasubiResourcePath[]           = "item/item_nasubi";
	const char sJennyWallHabakiResourcePath[]          = "jenny/wall_habaki";
	const char sJennyWallTopResourcePath[]             = "jenny/wall_top";
	const char sJennyWallTileResourcePath[]            = "jenny/wall_tile";
	const char sParentPapaToyboxResourcePath[]         = "parent/papa_toybox";
	const char sParentPapaToybox2ResourcePath[]        = "parent/papa_toybox2";
	const char sParentRecordsResourcePath[]            = "parent/records";
	const char sFxGiccoTwinb01ResourcePath[]           = "fx_gicco_twinb01";
	const char sFxGiccoTwinb02ResourcePath[]           = "fx_gicco_twinb02";
	const char sFxGiccoTwinb03ResourcePath[]           = "fx_gicco_twinb03";
	const char sItemDensetuTaneResourcePath[]          = "item/densetu_tane";
	const char sFxBackyardRainResourcePath[]           = "fx_backyard_rain";
	const char sRoukaCoUnderResourcePath[]             = "rouka/co_under";
	const char sRoukaTakiSResourcePath[]               = "rouka/taki_s";
	const char sRoukaTakiSCResourcePath[]              = "rouka/taki_s_c";
	const char sParentRecplayerNeedleResourcePath[]    = "parent/recplayer_needle";
	const char sFxAlien01ResourcePath[]                = "fx_alien01";
	const char sBackyardAriSuResourcePath[]            = "backyard/ari_su";
	const char sKitchenCoUnderResourcePath[]           = "kitchen/co_under";
	const char sNpcBirdResourcePath[]                  = "npc/bird";
	const char sNpcTaoSResourcePath[]                  = "npc/tao_s";
	const char sParentTBlocksResourcePath[]            = "parent/t_blocks";
	const char sKitchenMituBinResourcePath[]           = "kitchen/mitu_bin";
	const char sJennySanpuBinResourcePath[]            = "jenny/sanpu_bin";
	const char sTestSpiderJanks1ResourcePath[]         = "test/spider_janks1";
	const char sChibihouseRankingNoResourcePath[]      = "chibihouse/ranking_no";
	const char sChibihouseRankingBoardResourcePath[]   = "chibihouse/ranking_board";
	const char sSoukoSunRagResourcePath[]              = "souko/sun_rag";
	const char sSoukoSunRouResourcePath[]              = "souko/sun_rou";
	const char sLivingTvNews2ResourcePath[]            = "living/tv_news2";
	const char sBackyardKumoAmeResourcePath[]          = "backyard/kumo_ame";
	const char sAllYogore30kResourcePath[]             = "all/yogore_30k";
	const char sAllYogore50kResourcePath[]             = "all/yogore_50k";
	const char sHaisuiAnaAResourcePath[]               = "haisui/ana_a";
	const char sKitchenBgResourcePath[]                = "kitchen/bg";
	const char sKitchenFruitResourcePath[]             = "kitchen/fruit";
	const char sKitchenHaeResourcePath[]               = "kitchen/hae";
	const char sFxMspiderEyeResourcePath[]             = "fx_mspider_eye";
	const char sTestMtJank1ResourcePath[]              = "test/mt_jank1";
	const char sTestMtJank2ResourcePath[]              = "test/mt_jank2";
	const char sTestMtJank3ResourcePath[]              = "test/mt_jank3";
	const char sTestMtJank4ResourcePath[]              = "test/mt_jank4";
	const char sTestMtJank5ResourcePath[]              = "test/mt_jank5";
	const char sTestMtJank6ResourcePath[]              = "test/mt_jank6";
	const char sTestMtJank7ResourcePath[]              = "test/mt_jank7";
	const char sTestMtJank8ResourcePath[]              = "test/mt_jank8";
	const char sTestMtJank9ResourcePath[]              = "test/mt_jank9";
	const char sItemKure1ResourcePath[]                = "item/kure1";
	const char sItemKure2ResourcePath[]                = "item/kure2";
	const char sItemKure3ResourcePath[]                = "item/kure3";
	const char sItemKure4ResourcePath[]                = "item/kure4";
	const char sItemKure5ResourcePath[]                = "item/kure5";
	const char sParentGboxBigResourcePath[]            = "parent/gbox_big";
	const char sParentGboxSmallResourcePath[]          = "parent/gbox_small";
	const char sLivingFafaOmcResourcePath[]            = "living/fafa_omc";
	const char sTestSp3BomResourcePath[]               = "test/sp3_bom";
	const char sBackyardWindowKResourcePath[]          = "backyard/window_k";
	const char sLivingTvSunaResourcePath[]             = "living/tv_suna";
	const char sBackyardJimenRyuukiResourcePath[]      = "backyard/jimen_ryuuki";
	const char sBackyardFuneTutiDResourcePath[]        = "backyard/fune_tuti_d";
	const char sChibihouseElect10ResourcePath[]        = "chibihouse/elect10";
	const char sChibihouseElect2ResourcePath[]         = "chibihouse/elect2";
	const char sChibihouseElect3ResourcePath[]         = "chibihouse/elect3";
	const char sChibihouseElect4ResourcePath[]         = "chibihouse/elect4";
	const char sChibihouseElect5ResourcePath[]         = "chibihouse/elect5";
	const char sChibihouseElect6ResourcePath[]         = "chibihouse/elect6";
	const char sChibihouseElect7ResourcePath[]         = "chibihouse/elect7";
	const char sChibihouseElect8ResourcePath[]         = "chibihouse/elect8";
	const char sChibihouseElect9ResourcePath[]         = "chibihouse/elect9";
	const char sLivingHodokeItoResourcePath[]          = "living/hodoke_ito";
	const char sChibihouseDenti2conResourcePath[]      = "chibihouse/denti2con";
	const char sAllYogoreKureyonKResourcePath[]        = "all/yogore_kureyon_k";
	const char sBackyardGrassResourcePath[]            = "backyard/grass";
	const char sLivingMotherWallResourcePath[]         = "living/mother_wall";
	const char sLivingMotherMapResourcePath[]          = "living/mother_map";
	const char sLivingProtectorResourcePath[]          = "living/protector";
	const char sLivingApronResourcePath[]              = "living/apron";
	const char sLivingCurtainResourcePath[]            = "living/curtain";
	const char sParentTBlocks2ResourcePath[]           = "parent/t_blocks2";
	const char sRoukaNwingDaiResourcePath[]            = "rouka/nwing_dai";
	const char sKitchenShelfLHaResourcePath[]          = "kitchen/shelf_l_ha";
	const char sKitchenShelfLHbResourcePath[]          = "kitchen/shelf_l_hb";
	const char sKitchenClockLResourcePath[]            = "kitchen/clock_l";
	const char sKitchenClockSResourcePath[]            = "kitchen/clock_s";
	const char sKitchenShelfLRResourcePath[]           = "kitchen/shelf_l_r";
	const char sKitchenHamburgKResourcePath[]          = "kitchen/hamburg_k";
	const char sJennyMiniBoxResourcePath[]             = "jenny/mini_box";
	const char sJennyMiniHikidasiResourcePath[]        = "jenny/mini_hikidasi";
	const char sJennyMapKure1ResourcePath[]            = "jenny/map_kure1";
	const char sJennyMapKure2ResourcePath[]            = "jenny/map_kure2";
	const char sJennyMapKure3ResourcePath[]            = "jenny/map_kure3";
	const char sJennyMapKure4ResourcePath[]            = "jenny/map_kure4";
	const char sJennyMapKure5ResourcePath[]            = "jenny/map_kure5";
	const char sFxTompCam2dResourcePath[]              = "fx_tomp_cam2d";
	const char sFxMummiformResourcePath[]              = "fx_mummiform";
	const char sFxPirateboatResourcePath[]             = "fx_pirateboat";
	const char sFxMspider01ResourcePath[]              = "fx_mspider_01";
	const char sFxMspider02ResourcePath[]              = "fx_mspider_02";
	const char sFxTmUniteResourcePath[]                = "fx_tm_unite";
	const char sJennyJClockLResourcePath[]             = "jenny/j_clock_l";
	const char sJennyJClockSResourcePath[]             = "jenny/j_clock_s";
	const char sSoukoSClockLResourcePath[]             = "souko/s_clock_l";
	const char sSoukoSClockSResourcePath[]             = "souko/s_clock_s";
	const char sRoukaRClockLResourcePath[]             = "rouka/r_clock_l";
	const char sRoukaRClockSResourcePath[]             = "rouka/r_clock_s";
	const char sJennyCartainResourcePath[]             = "jenny/cartain";
	const char sJennyTvSampooResourcePath[]            = "jenny/tv_sampoo";
	const char sJennyBagHimo2ResourcePath[]            = "jenny/bag_himo2";
	const char sJennyBagHimo3ResourcePath[]            = "jenny/bag_himo3";
	const char sJennyBagHimo4ResourcePath[]            = "jenny/bag_himo4";
	const char sJennyHeitaiTaoreResourcePath[]         = "jenny/heitai_taore";
	const char sJennyShelfBooksResourcePath[]          = "jenny/shelf_books";
	const char sJennyMoveBook1ResourcePath[]           = "jenny/move_book1";
	const char sJennyMoveBook2ResourcePath[]           = "jenny/move_book2";
	const char sJennySanpuBin2ResourcePath[]           = "jenny/sanpu_bin2";
	const char sSoukoBirdcageShortcutResourcePath[]    = "souko/birdcage_shortcut";
	const char sSoukoBirdcageShortcut2ResourcePath[]   = "souko/birdcage_shortcut2";
	const char sItemMiniFunky1ResourcePath[]           = "item/mini_funky1";
	const char sNpcSpider4ResourcePath[]               = "npc/spider4";
	const char sNpcZobinBm2ResourcePath[]              = "npc/zobin_bm2";
	const char sNpcZobinBm3ResourcePath[]              = "npc/zobin_bm3";
	const char sParentFunkyStageResourcePath[]         = "parent/funky_stage";
	const char sItemMiniFunkyButiResourcePath[]        = "item/mini_funky_buti";
	const char sItemTaneFunkyResourcePath[]            = "item/tane_funky";
	const char sNpcFunkyF3ResourcePath[]               = "npc/funky_f3";
	const char sKitchenShelfLRbResourcePath[]          = "kitchen/shelf_l_rb";
	const char sRoukaSpiderBResourcePath[]             = "rouka/spider_b";
	const char sNpcTyouBbResourcePath[]                = "npc/tyou_bb";
	const char sRoukaWHashigoResourcePath[]            = "rouka/w_hashigo";
	const char sRoukaWRopeResourcePath[]               = "rouka/w_rope";
	const char sRoukaTeRopeAResourcePath[]             = "rouka/te_rope_a";
	const char sRoukaTeRopeBResourcePath[]             = "rouka/te_rope_b";
	const char sRoukaBgResourcePath[]                  = "rouka/bg";
	const char sRoukaLightResourcePath[]               = "rouka/light";
	const char sSoukoCoUnderResourcePath[]             = "souko/co_under";
	const char sSoukoSUnderObjResourcePath[]           = "souko/s_under_obj";
	const char sParentClockLResourcePath[]             = "parent/clock_l";
	const char sParentClockSResourcePath[]             = "parent/clock_s";
	const char sParentClockFResourcePath[]             = "parent/clock_f";
	const char sParentClockCuckooResourcePath[]        = "parent/clock_cuckoo";
	const char sParentBg01ResourcePath[]               = "parent/bg01";
	const char sFxSpider02ResourcePath[]               = "fx_spider02";
	const char sSoukoObjAResourcePath[]                = "souko/obj_a";
	const char sSoukoObjBResourcePath[]                = "souko/obj_b";
	const char sSoukoObjCResourcePath[]                = "souko/obj_c";
	const char sRoukaCObjResourcePath[]                = "rouka/c_obj";
	const char sRouka2fObjResourcePath[]               = "rouka/2f_obj";
	const char sRouka1fSRopeResourcePath[]             = "rouka/1f_s_rope";
	const char sRouka2fSRopeResourcePath[]             = "rouka/2f_s_rope";
	const char sParentIgaguBookResourcePath[]          = "parent/igagu_book";
	const char sParentFfHanaResourcePath[]             = "parent/ff_hana";
	const char sLivingTanaueRopeResourcePath[]         = "living/tanaue_rope";
	const char sNpcFunkyF2BResourcePath[]              = "npc/funky_f2_b";
	const char sNpcFunkyF3BResourcePath[]              = "npc/funky_f3_b";
	const char sParentFunkyStage2ResourcePath[]        = "parent/funky_stage2";
	const char sBackyardCamBackyardResourcePath[]      = "backyard/cam_backyard";
	const char sItemSakiFlower2ResourcePath[]          = "item/saki_flower2";
	const char sItemSakiFlower3ResourcePath[]          = "item/saki_flower3";
	const char sLivingFlowerKazari1ResourcePath[]      = "living/flower_kazari1";
	const char sLivingFlowerKazari2ResourcePath[]      = "living/flower_kazari2";
	const char sLivingFlowerKazari3ResourcePath[]      = "living/flower_kazari3";
	const char sNpcTaoHunburgResourcePath[]            = "npc/tao_hunburg";
	const char sNpcItoArmyResourcePath[]               = "npc/ito_army";
	const char sHouseRItoHMainResourcePath[]           = "house_r/ito_h_main";
	const char sKitchenTaoCollisionBox01ResourcePath[] = "kitchen/tao_collision_box01";
	const char sKitchenTaoCollisionBox02ResourcePath[] = "kitchen/tao_collision_box02";
	const char sKitchenTaoCollisionBox03ResourcePath[] = "kitchen/tao_collision_box03";
	const char sKitchenBbCollisionBox01ResourcePath[]  = "kitchen/bb_collision_box01";
	const char sAllYogoreFfResourcePath[]              = "all/yogore_ff";
	const char sNpcEtTakoResourcePath[]                = "npc/et_tako";
	const char sNpcDryFrogResourcePath[]               = "npc/dry_frog";
	const char sLivingSpiderBLResourcePath[]           = "living/spider_b_l";
	const char sKitchenFrypanResourcePath[]            = "kitchen/frypan";
	const char sRoukaSpiderKakoiResourcePath[]         = "rouka/spider_kakoi";
	const char sParentFfHanatabaResourcePath[]         = "parent/ff_hanataba";
	const char sKitchenShelfLObjResourcePath[]         = "kitchen/shelf_l_obj";
	const char sFxScream2dResourcePath[]               = "fx_scream2d";
	const char sFxProse2dResourcePath[]                = "fx_prose2d";
	const char sFxCaution2dResourcePath[]              = "fx_caution2d";
	const char sFxTmTakeoffResourcePath[]              = "fx_tm_takeoff";
	const char sFxMspiderEye2dResourcePath[]           = "fx_mspider_eye2d";
	const char sNpcFunkyFAllResourcePath[]             = "npc/funky_f_all";
	const char sNpcFunkyF2AllResourcePath[]            = "npc/funky_f2_all";
	const char sNpcFunkyF3AllResourcePath[]            = "npc/funky_f3_all";
	const char sJennyJObj1ResourcePath[]               = "jenny/j_obj1";
	const char sJennyJObj2ResourcePath[]               = "jenny/j_obj2";
	const char sItemCDenchiResourcePath[]              = "item/c_denchi";
	const char sChibihouseJunkIreruResourcePath[]      = "chibihouse/junk_ireru";
	const char sBackyardFullMoonResourcePath[]         = "backyard/full_moon";
	const char sBackyardBrankoEndResourcePath[]        = "backyard/branko_end";
	const char sAllRakugakiDekaResourcePath[]          = "all/rakugaki_deka";
	const char sKitchenDenkiconroResourcePath[]        = "kitchen/denkiconro";
	const char sBackyardTaoBag2ResourcePath[]          = "backyard/tao_bag2";
	const char sNpcTHanamizuResourcePath[]             = "npc/t_hanamizu";
	const char sNpcSpIto2ResourcePath[]                = "npc/sp_ito2";
	const char sNpcTaoSlpResourcePath[]                = "npc/tao_slp";
	const char sLivingTvKeath2ResourcePath[]           = "living/tv_keath2";
	const char sNpcCusDecaroboResourcePath[]           = "npc/cus_decarobo";
	const char sNpcDecaroboHandResourcePath[]          = "npc/decarobo_hand";
	const char sJennyJTrashboxResourcePath[]           = "jenny/j_trashbox";
	const char sParentPoleRopeResourcePath[]           = "parent/pole_rope";
	const char sNpcUfo2ResourcePath[]                  = "npc/ufo2";
	const char sKitchenPotResourcePath[]               = "kitchen/pot";
	const char sKitchenPotRopeResourcePath[]           = "kitchen/pot_rope";
	const char sParentDatuTamaResourcePath[]           = "parent/datu_tama";
	const char sRoukaArmyHeri2ResourcePath[]           = "rouka/army_heri2";
	const char sRoukaArmyRun1ResourcePath[]            = "rouka/army_run1";
	const char sRoukaArmyRun2ResourcePath[]            = "rouka/army_run2";
	const char sJennyPeetsFootResourcePath[]           = "jenny/peets_foot";
	const char sBackyardHockAnaResourcePath[]          = "backyard/hock_ana";
	const char sBackyardHockAnaKaidanResourcePath[]    = "backyard/hock_ana_kaidan";
	const char sFxAlienEndResourcePath[]               = "fx_alien_end";
	const char sFxDecaEndResourcePath[]                = "fx_deca_end";
	const char sNpcZobin2ResourcePath[]                = "npc/zobin2";
	const char sParentTpStageResourcePath[]            = "parent/tp_stage";
	const char sJennyZobinDaiResourcePath[]            = "jenny/zobin_dai";
	const char sKitchenTaoCollisionBox04ResourcePath[] = "kitchen/tao_collision_box04";
	const char sNpcSunUfoResourcePath[]                = "npc/sun_ufo";
	const char sTestShortplayTitleResourcePath[]       = "test/shortplay_title";
	const char sChargeFx01ResourcePath[]               = "charge_fx01";
	const char sCbPropellerLv2ResourcePath[]           = "cb_propeller_lv2";
	const char sMaruBatuResourcePath[]                 = "maru_batu";
	const char sExclamationResourcePath[]              = "exclamation";
	const char sKisekaePartsKeathResourcePath[]        = "kisekae/parts_keath";
	const char sKisekaePartsKaeruResourcePath[]        = "kisekae/parts_kaeru";
	const char sKisekaePartsKegaResourcePath[]         = "kisekae/parts_kega";
	const char sKisekaePartsObakeResourcePath[]        = "kisekae/parts_obake";
	const char sKisekaePartsPajamaResourcePath[]       = "kisekae/parts_pajama";
	const char sKisekaePartsSuperResourcePath[]        = "kisekae/parts_super";
	const char sKisekaePartsPajama01ResourcePath[]     = "kisekae/parts_pajama_01";
	const char sKisekaePartsPajama02ResourcePath[]     = "kisekae/parts_pajama_02";
	const char sTitleIconIconArmyResourcePath[]        = "title_icon/icon_army";
	const char sTitleIconIconFafaResourcePath[]        = "title_icon/icon_fafa";
	const char sTitleIconIconFunkyResourcePath[]       = "title_icon/icon_funky";
	const char sTitleIconIconHockResourcePath[]        = "title_icon/icon_hock";
	const char sTitleIconIconKeathResourcePath[]       = "title_icon/icon_keath";
	const char sTitleIconIconPeetsResourcePath[]       = "title_icon/icon_peets";
	const char sTitleIconIconSunmidaResourcePath[]     = "title_icon/icon_sunmida";
	const char sTitleIconIconTrexResourcePath[]        = "title_icon/icon_trex";
	const char sTitleIconIconZobinResourcePath[]       = "title_icon/icon_zobin";
	const char sTitleIconIconArmy2ResourcePath[]       = "title_icon/icon_army2";
	const char sKisekaePartsTaoResourcePath[]         = "kisekae/parts_tao";
	const char sTestWardrobeResourcePath[]            = "test/wardrobe";

XObjSource gObjectSources[] = {
	{ "cb_robo", 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sKitchenMapResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sKitchenReizoukoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenGasDaiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenSinkUnder1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenSinkMidleResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenTaoEResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenPlantBoxResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenCounter1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenCounter2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenTableResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenShelfLResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenDoorResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenWallWalkResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcConsentmanResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcTonpyResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sKitchenWindowSResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ "cb_plug", 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sTestBox4ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sTestBox8ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sTestBox12ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sTestBoxLiftResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sTestRopeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sTestLadderResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sRoukaMapResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sRoukaDoorJResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaDoorKResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaDoorPamaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaEntranceResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaKaidan1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaKaidan2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaKaidan3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaMatResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sRoukaRoCounterResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaRoPlantResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaRoSofaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTableKakuResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTableMaruResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenChair1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTesuri2fResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaWall12fResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaWall1fResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sRoukaWall2fResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sPushobj3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardMapResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ "coin_c", 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ "coin_g", 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ "coin_s", 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardBaseResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sBackyardBgResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sBackyardDoorResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardFenceResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardWallResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoMapResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sSoukoWallResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sPushobj1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sPushobj2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemPenResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ "socket", 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sKitchenWallResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemDekaDenchiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihousePcGamenResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sChibihouseGachaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihouseAirResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyMapResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sJennyBedResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sJennyChair1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyChair2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenWindowResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyClosetResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sJennyDeskResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyDoorResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyDresserResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyLight1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyMatResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyShelfResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyTableResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyWallResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyWallShelfResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyWindow1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyWindow2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentMapResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentBarwallLResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentBarwallMResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentBarwallRResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentBedResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentChairResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentClosetResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentCornershelfResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentDresserResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentSofaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentTableResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sMapJumpBoxResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ "npc/tao", 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentWallResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentWallshelfResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentWindowResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcHockResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sSoukoKaidanResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoDcDaiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingShelfDanBoxResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingShelfDanDaiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingDoorResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaDoorLResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaDoorLDamiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihouseCusCbPrLv1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sChibihouseGachaPon1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingWallShelf1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingTelResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingTelCounterResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingTelJResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingWallCounterResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingShelfWResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingWallUpResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingShelfSlopeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingFlowerclockResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sSoukoHasiraResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingLivingSkyResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingButterflyResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingFlowerBedResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sRoukaKaidan2hResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaKaidan3hResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoWineBottleResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sRoukaHashiraTResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaHashiraEResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaWallUnderResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaFloor2fResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihouseMapResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sChibihouseWallResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sChibihouseChiPcResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihouseChiSocketResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihouseChiEResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaDoorEResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaWall2fUnderResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibiHouseResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaWindowResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemFrogRingResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sChibiJumpResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ "energyb", 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcJennyResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingMapResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingDanroResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingShelfBResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingShelfDanResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingSidetableResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingSofa1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingSofa2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingStandlightResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingTableResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingTvResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingWallResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingWindowResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sHouseRChibiHMainResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sHouseRChibiHDaiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcMamaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingSofaHashigoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingBooks1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingBooks2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingSofaCu1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingSofaCu2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingFlowerpot2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingShelfDanBookResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingTakoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingOekakiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sHouseRChibiHHasigoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcArmy1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcArmy2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sHouseRChibiHDoor1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sHouseRChibiHDoor2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sDoorroomResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sDoorroomDokanResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemSnackBoneResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemJunkUnkoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemJunkAResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sTestTestRingResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingTableFootResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingMatResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingTubResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sPlugStartResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcDecaRoboResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcKeathResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sChibihouseDenti1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihouseDenti2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcPapaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sHouseRChibiHHasiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sHouseRChibiHWarpResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcFafaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcSpider1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ "npc/ufo", 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemBrushResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingSakanaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingKaigaJResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemCapsuleResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardAriResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingBgResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardAriDaidaiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sBackyardTreeBResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sBackyardKadanResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sPlugDokodakadoorResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingBbDanResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingTrashboxResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingTaoDishResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoSpiderSuBResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingSuisouR2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingSakanaAwaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoKumoHimoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ "npc/et1", 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemKamiKuzuResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemCookieKakeraResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingTvKeathResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sTestDust0ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sTestDust1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingSakanaEsaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingTanaAnaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sTestSpItoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingWallCornerResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingCoUnderResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sTestJennyHandResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingWindowKazariResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemJunkBResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemJunkCResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingGabyouResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sBackyardSkyResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sTestFlyRoot1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcArmy3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcNWing1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcNWing2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sRoukaPlanterResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemSpoonResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sTestNMatoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyCubeBoxResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyCubeLResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyCubeOResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyCubeVResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyCubeEResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyCuAnimal1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyCuAnimal2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemMagCupResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcSunMidaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcSunDaiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingCakeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ "npc/bb", 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemBaraResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingOrangeGlassResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingOrangeBinResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingChamGlassResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingChamBinResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingVaseResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingChickenResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingLivingCamResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcSpider2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingCakeDishResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingKazariTResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenDoorRResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingAsiatoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingHatResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingKazariWResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingBirthdayCResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingBoneRResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenDoorDamiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemCosObakeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenTutiHutaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenSinkHiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenSinkHiUnderResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenTrashboxResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenHammockResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaShoesResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sKitchenSink2hiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaSRopeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingHappyBoxResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingConsentmanAtariResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemTonpyItemAResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemTonpyItemBResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemTonpyItemCResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemHockTizuResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sHowToChibiroboCbBookResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sFxCbHouse01ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingCbCoverResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingHappyHeartResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllYogoreCookOResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllYogoreWaterResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllYogoreOilResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingCamLivingResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sHouseRChibiHMakuResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenYakanResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenTeacupResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenTeapotResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenMilkBinResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenSugarBinResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenCookieDishResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenCbTeasetResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemBlockSugarResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemFlowerCookieResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenSinkSShelfResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllSpiderKageResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenSinkDishResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenHukinResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sKitchenSinkKagoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingTvSwitchResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenTeacupSResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingCrackerResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxDecaSpark01ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingTvNewsResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingTvKageResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingCbBook2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxTompCry01ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingSw1planterResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingSw2planterResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenBbNikkiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenBbPenResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingWallUnderResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sHowToChibiroboMapResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sFxTompScare01ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingDoorKResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemCrackerResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingCrackerGomiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingFClockHbResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingFClockHsResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingFClockFResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingPresentBoxResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingBgTreeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxSpider01ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingChickenBoneResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingCakeAtoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaCupArmyResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoWineBottleAResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoWineBottleBResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenSink3hiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyTana1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemChipResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingWindowWareResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingWindowHirakuResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcArmy4ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcPrimoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentPartitionResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentPartition2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentPartition3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentPartition5ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentWallshelf2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentWallshelf3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentWallshelf4ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentWallshelf5ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentWallshelf6ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentSwitchResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sHouseRDream1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentSidetableResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentLadderResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentCloset2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentDoorResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentTable2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentChestResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentChest2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentBlindRResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentBlindLResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentBlindhandleRResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentBlindhandleLResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentSewingmachineResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyFlowerLampResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemReceiptResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentMamaPenResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentMamaCalculatorResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentBillsResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentReceiptResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentAccountBookResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ "ufo/map", 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sUfoWallResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemTyuusyakiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenHamburgResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyJennyTvResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcFunkyFResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcTRexResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sAllUfoWarpResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllCllPapaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllCllMamaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenGasPlateResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentDoorbellResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentDoorwindowResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentDoorwindow2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentPlanterResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentPlanter2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentPoleResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentPole2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentPole3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentPole4ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentPole5ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentPole6ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentPole7ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentPole8ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentRecplayerResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentRecspeakerLResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentRecspeakerRResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentStpoleResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentStpole2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemTonpyItemEResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemTonpyItemFResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemTonpyItemGResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemTonpyItemHResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemTonpyItemIResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemTonpyItemJResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemTonpyItemKResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemTonpyItemLResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemTonpyItemMResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemTonpyItemNResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemTonpyItemOResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemTonpyItemPResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemTonpyItemQResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemTonpyItemRResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemTonpyItemSResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemTonpyItemTResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemTonpyItemUResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sBackyardWindow1lResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardWindow2lResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemLetterResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentDoorswitchResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyAsiba1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyAsiba2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyAsiba3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyCastleDoorResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyHeitai1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyKaidanResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyHikidasiCloseResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyHikidasiOpenResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaDoorPamaHResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaRopeDoorResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaRopeNobuResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyAsibaS1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyCastleHasigoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyHariLResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyHariSResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyHeitai2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyHeitai3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyHeitai4ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemDenchi1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemDenchi2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemDenchi3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemPeetsKutuResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemHocyoukiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyAsibaKado1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyAsiba4ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingMituFlowerResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyHariL2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcPapaSleepResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcMamaSleepResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcJennySleepResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sAllSaSealResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentSkirtingBoardResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentSkirtingBoard2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentSkirtingBoardpResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentRecplayerCableResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentBed2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTakiObjResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaCeilingResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyTvGamenResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentBedStayResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sAllYogoreTaiyaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllYogoreKureyonResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllYogoreToyResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcFrogResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentCornershelfLadderResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardWindowWareResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardJagutiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyHasiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenHamPanUpResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenHamPanDownResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyHeitai5ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxLeticwarp01ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardHoseResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaNebukuroResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentCamPapamamaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyCamJennyResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllEnjoyLightResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcZobinResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcPeetsResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sJennyHeitaiTaikoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyHeitaiRappaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyJennyHimoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyJennyBagResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxZactGiccoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxZactTaoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxZactFrogResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxZactInjuredResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxZactPajamasResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentSaisunBoxResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sBackyardKumoUfoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingHappyHeart2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyBedTopResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAll1dayTimerResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaDoorStopResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemRexToothResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyZobinBoxResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sRoukaKaidanAtamaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllLeafResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaDoorKazariResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyHeitaiGuitarResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcZobinBm1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemTaneAResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemBbPenResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemInkResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemKareFlowerResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyPresentHanaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingSunmidaskyResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemKayakuResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemGoggleResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemLoveLetterResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemTugResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemTicketResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemHoutaiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemBlockResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemRecordGiccioResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemRecordFfResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingWallCHResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemNwingItemResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemArmyPhotoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaCounterHResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sTestFlyRoot2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcCar1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcCar2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcFunkyF2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sSoukoShelfResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sSoukoChestResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoRengaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenKamareBbResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sSoukoShelfDamiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sKitchenTutiAnaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcHockShipResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemLeftFootResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentSuitCaseResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemKairozuResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemDekaDenchiFullResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyHagurumaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyPlanterZobinResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyPlanterJenny1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyPlanterJenny2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoUfoBallResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoTimeMcResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemPajamaKijiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyPresentHanaZobinResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllFlower1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllFlower2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllFlower3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllJunkTreeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllCoinTreeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllHappyTreeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllMameTreeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennySanpuHouseResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemKareFlowerMituResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemSakiFlower1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenJaguchiHResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenTutiHutaDResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcTTrackResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcArmy5ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sUfoWall2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenWallTonneruResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenWallTWareResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyToyDenwaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyToyPianoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoExitResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingDensetuHanaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sHaisuiMapResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sHaisuiPipeAResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sHaisuiHashigoBResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sHaisuiHashigoCResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sHaisuiHashigoDResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sUfoGroundResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoGround2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoGround3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoBallRotResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoGame1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoGame2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoGame3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoGame4ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoGameBotanResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoGameP1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoGameP2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoGameP3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoGameP4ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoWall3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sTestCarRoot1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sTestCarRoot2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoGameBotan2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sTestNMato2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenSinkHashigoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTutaAResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTutaARaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTutaARbResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyChibiIsuResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTutaBResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTutaBRResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTutaCResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTutaCRResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTutaDResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTutaDRResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenConroResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenKingyoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenWPlanterResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTakiHashiAResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTakiHashiBResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyKenban1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyKenban2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentTpMike1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentTpMike2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyHouseHimoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyBagHimoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTakiKakoiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaArmyIta1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaArmyIta2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaArmyHeri1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoUfoBedResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyJennyDressResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxKitchenburnerResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxWarpzoneResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoUfoBed2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoUfoBed3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sUfoUfoBed4ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcNasubiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sAllTonpyItemKageResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaRaceItaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sHaisuiSotoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sHaisuiNezumiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sHaisuiHashigoFResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sHaisuiRopeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sHaisuiPipeBResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyJennyDress2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyJennyDress3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sHaisuiKabeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sSoukoBirdcageResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoBoardResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoNailResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoVRopeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sHaisuiWaterResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemPajamaKiji2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemPajamaKiji3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentTKazan1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentTKazan2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentTKazan3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentTKazan4ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentTKazan5ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentTKazan6ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemTBlock1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemTBlock2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemTBlock3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemTBlock4ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemTBlock5ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemTBlock6ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemCarItemResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sBackyardTHashigoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardBrankoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardBRopeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemPapaYubiwaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentTKageResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxZactGhostResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentFrogHagataResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemKusaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaBebeBinsenResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemSekkeizuResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemItemFrogResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxPcBuy01ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyJennyBookResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyPlanterHachiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyKureyonResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxCbdoor01ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxTmBarrierResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennySuberidai1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennySuberidai2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingAlbumTResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingAlbumYResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingTableStandResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingKaizouPoperResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentWeldingMaskpResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentWeldingMaskmResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentWeldingMaskjResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentWeldingMasktResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentWeldingTorchResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentWeldingToolboxResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentWeldingToolsResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentDrafterResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenGasHashigoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenGasRopeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentCaseJanksResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sYaneTorinosuResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardKusaTutiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingKeathOmcResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sTitleIconIconBbResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTaoBagResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaKokubanResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentDcfootSpiderResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllTakarabakoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sCbCannonLv2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sCbRadarResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingCbKaiPoperResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sHaisuiRoom1CResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sFxDecaEnergyResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxDecaBatteryResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFx3pu01ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxPapa01ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenKankisenResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemTamagottiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcDecaRobo2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sKitchenToasterResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenSenzaiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenSuponjiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sKitchenJamResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sKitchenGusUpResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllYogoreHSResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllYogoreHKResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcSpider3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcSunKaoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sKitchenPuroperaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyCastleHasigo2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllYogoreHitoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllYogoreHunResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllYogoreShimiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxArmySgtResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyJennyKagamiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingTamagottiGamenResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sKitchenKankisenAsibaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingLightRopeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingAudioResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingSibariMamaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingSibariPapaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingSibariTaoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingSibariJennyResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemAkikanResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemCandyGomiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemOkasiGomi1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemOkasiGomi2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemItemNasubiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyWallHabakiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyWallTopResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyWallTileResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentPapaToyboxResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentPapaToybox2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentRecordsResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxGiccoTwinb01ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxGiccoTwinb02ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxGiccoTwinb03ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemDensetuTaneResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxBackyardRainResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaCoUnderResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTakiSResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTakiSCResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentRecplayerNeedleResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxAlien01ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardAriSuResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenCoUnderResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcBirdResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcTaoSResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentTBlocksResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sKitchenMituBinResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennySanpuBinResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sTestSpiderJanks1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sChibihouseRankingNoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihouseRankingBoardResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sSoukoSunRagResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sSoukoSunRouResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingTvNews2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardKumoAmeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllYogore30kResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllYogore50kResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sHaisuiAnaAResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sKitchenBgResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sKitchenFruitResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenHaeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxMspiderEyeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sTestMtJank1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sTestMtJank2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sTestMtJank3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sTestMtJank4ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sTestMtJank5ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sTestMtJank6ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sTestMtJank7ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sTestMtJank8ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sTestMtJank9ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemKure1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemKure2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemKure3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemKure4ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemKure5ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentGboxBigResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentGboxSmallResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingFafaOmcResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sTestSp3BomResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sBackyardWindowKResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingTvSunaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardJimenRyuukiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sBackyardFuneTutiDResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihouseElect10ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihouseElect2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihouseElect3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihouseElect4ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihouseElect5ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihouseElect6ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihouseElect7ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihouseElect8ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihouseElect9ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingHodokeItoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihouseDenti2conResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllYogoreKureyonKResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardGrassResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingMotherWallResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingMotherMapResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingProtectorResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingApronResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingCurtainResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentTBlocks2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaNwingDaiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenShelfLHaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenShelfLHbResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenClockLResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenClockSResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenShelfLRResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenHamburgKResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyMiniBoxResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyMiniHikidasiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyMapKure1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyMapKure2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyMapKure3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyMapKure4ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyMapKure5ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxTompCam2dResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxMummiformResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxPirateboatResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxMspider01ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxMspider02ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxTmUniteResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyJClockLResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyJClockSResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoSClockLResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoSClockSResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaRClockLResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaRClockSResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sJennyCartainResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyTvSampooResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyBagHimo2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyBagHimo3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyBagHimo4ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyHeitaiTaoreResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyShelfBooksResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyMoveBook1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyMoveBook2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennySanpuBin2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoBirdcageShortcutResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sSoukoBirdcageShortcut2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemMiniFunky1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcSpider4ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcZobinBm2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcZobinBm3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentFunkyStageResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sItemMiniFunkyButiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemTaneFunkyResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcFunkyF3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sKitchenShelfLRbResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaSpiderBResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcTyouBbResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sRoukaWHashigoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaWRopeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTeRopeAResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaTeRopeBResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaBgResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaLightResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoCoUnderResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoSUnderObjResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentClockLResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentClockSResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentClockFResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentClockCuckooResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentBg01ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxSpider02ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoObjAResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoObjBResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sSoukoObjCResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaCObjResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sRouka2fObjResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRouka1fSRopeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRouka2fSRopeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentIgaguBookResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentFfHanaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingTanaueRopeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcFunkyF2BResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcFunkyF3BResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentFunkyStage2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sBackyardCamBackyardResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemSakiFlower2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemSakiFlower3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingFlowerKazari1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingFlowerKazari2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sLivingFlowerKazari3ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcTaoHunburgResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcItoArmyResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sHouseRItoHMainResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sKitchenTaoCollisionBox01ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenTaoCollisionBox02ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenTaoCollisionBox03ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenBbCollisionBox01ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllYogoreFfResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcEtTakoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcDryFrogResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingSpiderBLResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sKitchenFrypanResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaSpiderKakoiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentFfHanatabaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenShelfLObjResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxScream2dResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sFxProse2dResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxCaution2dResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxTmTakeoffResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxMspiderEye2dResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcFunkyFAllResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcFunkyF2AllResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcFunkyF3AllResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sJennyJObj1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyJObj2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sItemCDenchiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sChibihouseJunkIreruResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardFullMoonResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardBrankoEndResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sAllRakugakiDekaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenDenkiconroResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardTaoBag2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcTHanamizuResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcSpIto2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcTaoSlpResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sLivingTvKeath2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcCusDecaroboResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcDecaroboHandResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sJennyJTrashboxResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentPoleRopeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcUfo2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenPotResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenPotRopeResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sParentDatuTamaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sRoukaArmyHeri2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sRoukaArmyRun1ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sRoukaArmyRun2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sJennyPeetsFootResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardHockAnaResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sBackyardHockAnaKaidanResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxAlienEndResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sFxDecaEndResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sNpcZobin2ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sParentTpStageResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sJennyZobinDaiResourcePath, 0, 0x00000000, { 0x00000000, 0x00000001 } },
	{ sKitchenTaoCollisionBox04ResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ sNpcSunUfoResourcePath, 0, 0x00000000, { 0x00000000, 0x00000000 } },
	{ 0, 0, 0x00000000, { 0xFFFFFFFF, 0xFFFFFFFF } },
};

UnkObjData objTable[] = {
	{ &gObjectSources[0], 0x00000000, 0x000002C9, 3, 0, 0, 2, 0 },
	{ &gObjectSources[1], 0x00000000, 0x00000003, 0, 0, 0, 3, 0 },
	{ &gObjectSources[2], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[3], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[4], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[5], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[6], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[7], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[8], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[9], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[10], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[11], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[12], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[13], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[14], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[15], 0x00000000, 0x00000289, 3, 0, 0, 10, 10 },
	{ &gObjectSources[16], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[17], 0x00000000, 0x00000001, 0, 0, 0, 24, 0 },
	{ &gObjectSources[18], 0x00000000, 0x00000001, 0, 0, 0, 5, 11 },
	{ &gObjectSources[19], 0x00000000, 0x00000001, 0, 0, 0, 5, 13 },
	{ &gObjectSources[20], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[21], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[22], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[23], 0x00000000, 0x00000001, 0, 1, 15, 22, 0 },
	{ &gObjectSources[24], 0x00000000, 0x00000003, 0, 0, 0, 3, 0 },
	{ &gObjectSources[25], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[26], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[27], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[28], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[29], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[30], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[31], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[32], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[33], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[34], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[35], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[36], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[37], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[38], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[39], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[40], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[41], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[42], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[43], 0x00000000, 0x00000001, 0, 0, 0, 22, 30 },
	{ &gObjectSources[44], 0x00000000, 0x00000003, 0, 0, 0, 3, 0 },
	{ &gObjectSources[45], 0x00000000, 0x00000001, 0, 1, 15, 7, 2 },
	{ &gObjectSources[46], 0x00000000, 0x00000001, 0, 1, 15, 7, 0 },
	{ &gObjectSources[47], 0x00000000, 0x00000001, 0, 1, 15, 7, 1 },
	{ &gObjectSources[48], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[49], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[50], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[51], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[52], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[53], 0x00000000, 0x00000003, 0, 0, 0, 3, 0 },
	{ &gObjectSources[54], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[55], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[56], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[57], 0x00000000, 0x00000001, 0, 0, 0, 22, 30 },
	{ &gObjectSources[58], 0x00000000, 0x00000001, 0, 0, 0, 22, 30 },
	{ &gObjectSources[59], 0x00000000, 0x00000001, 0, 1, 15, 8, 1 },
	{ &gObjectSources[60], 0x00000000, 0x00000001, 0, 1, 15, 8, 2 },
	{ &gObjectSources[61], 0x00000000, 0x00000001, 0, 1, 15, 8, 3 },
	{ &gObjectSources[62], 0x00000000, 0x00000001, 0, 1, 15, 8, 4 },
	{ &gObjectSources[63], 0x00000000, 0x00000001, 0, 1, 15, 8, 5 },
	{ &gObjectSources[64], 0x00000000, 0x00000001, 0, 1, 15, 8, 6 },
	{ &gObjectSources[65], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[66], 0x00000000, 0x00000001, 0, 1, 15, 8, 7 },
	{ &gObjectSources[67], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[68], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[69], 0x00000000, 0x00000001, 0, 0, 0, 8, 8 },
	{ &gObjectSources[70], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[71], 0x00000000, 0x00000001, 0, 0, 0, 5, 7 },
	{ &gObjectSources[72], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[73], 0x00000000, 0x00000003, 0, 0, 0, 3, 0 },
	{ &gObjectSources[74], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[75], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[76], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[77], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[78], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[79], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[80], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[81], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[82], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[83], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[84], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[85], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[86], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[87], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[88], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[89], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[90], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[91], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[92], 0x00000000, 0x00000003, 0, 0, 0, 3, 0 },
	{ &gObjectSources[93], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[94], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[95], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[96], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[97], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[98], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[99], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[100], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[101], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[102], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[103], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[104], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ (u32)0, 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[106], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[107], 0x00000000, 0x00000081, 0, 0, 0, 14, 0 },
	{ &gObjectSources[108], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[109], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[110], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[111], 0x00000000, 0x00000081, 0, 0, 0, 10, 20 },
	{ &gObjectSources[112], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[113], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[114], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[115], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[116], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[117], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[118], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[119], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[120], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[121], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[122], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[123], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[124], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[125], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[126], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[127], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[128], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[129], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[130], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[131], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[132], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[133], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[134], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[135], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[136], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ (u32)0, 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[138], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[139], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[140], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[141], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[142], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[143], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[144], 0x00000000, 0x00000001, 0, 1, 15, 8, 50 },
	{ &gObjectSources[145], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[146], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[147], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[148], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[149], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[150], 0x00000000, 0x00000003, 0, 0, 0, 3, 0 },
	{ &gObjectSources[151], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[152], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[153], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[154], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[155], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[156], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[157], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[158], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[159], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[160], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[161], 0x00000000, 0x00000001, 0, 1, 15, 8, 0 },
	{ &gObjectSources[162], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[163], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[164], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[165], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[166], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[167], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[168], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[169], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[170], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[171], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[172], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[173], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[174], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[175], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[176], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[177], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[178], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[179], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[180], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[181], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[182], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[183], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[184], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[185], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[186], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[187], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[188], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[189], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[190], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[191], 0x00000000, 0x00000001, 0, 1, 15, 7, 200 },
	{ &gObjectSources[192], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[193], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[194], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[195], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[196], 0x00000000, 0x00000081, 0, 0, 0, 11, 0 },
	{ &gObjectSources[197], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[198], 0x00000000, 0x00000003, 0, 0, 0, 3, 0 },
	{ &gObjectSources[199], 0x00000000, 0x00000003, 0, 0, 0, 3, 0 },
	{ &gObjectSources[200], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[201], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[202], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[203], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[204], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[205], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[206], 0x00000000, 0x00002003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[207], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[208], 0x00000000, 0x00004003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[209], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[210], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[211], 0x00000000, 0x00004021, 0, 1, 20, 9, 0 },
	{ &gObjectSources[212], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[213], 0x00000000, 0x00004003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[214], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[215], 0x00000000, 0x00000081, 0, 0, 0, 12, 0 },
	{ &gObjectSources[216], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[217], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[218], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[219], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[220], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[221], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[222], 0x00000000, 0x00004003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[223], 0x00000000, 0x00004003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[224], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[225], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[226], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[227], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[228], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[229], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[230], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[231], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[232], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[233], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[234], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[235], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[236], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[237], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[238], 0x00000000, 0x00004001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[239], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[240], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[241], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[242], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[243], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[244], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[245], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[246], 0x00000000, 0x00000003, 0, 0, 0, 3, 0 },
	{ &gObjectSources[247], 0x00000000, 0x00000081, 0, 0, 0, 16, 0 },
	{ &gObjectSources[248], 0x00000000, 0x00000081, 4, 0, 0, 16, 0 },
	{ &gObjectSources[249], 0x00000000, 0x00004003, 0, 0, 0, 5, 1 },
	{ &gObjectSources[250], 0x00000000, 0x00004001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[251], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[252], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[253], 0x00000000, 0x00000001, 0, 1, 15, 8, 9 },
	{ &gObjectSources[254], 0x00000000, 0x00000001, 0, 1, 15, 8, 10 },
	{ &gObjectSources[255], 0x00000000, 0x00000001, 0, 1, 15, 8, 11 },
	{ &gObjectSources[256], 0x00000000, 0x00000001, 0, 1, 15, 8, 12 },
	{ &gObjectSources[257], 0x00000000, 0x00000001, 0, 1, 15, 8, 13 },
	{ &gObjectSources[258], 0x00000000, 0x00000001, 0, 1, 15, 8, 14 },
	{ &gObjectSources[259], 0x00000000, 0x00000001, 0, 1, 15, 8, 15 },
	{ &gObjectSources[260], 0x00000000, 0x00000001, 0, 1, 15, 7, 103 },
	{ &gObjectSources[261], 0x00000000, 0x00000001, 0, 1, 15, 7, 100 },
	{ &gObjectSources[262], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[263], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[264], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[265], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[266], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[267], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[268], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[269], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[270], 0x00000000, 0x00000021, 0, 0, 0, 22, 0 },
	{ &gObjectSources[271], 0x00000000, 0x00000081, 0, 0, 0, 15, 0 },
	{ &gObjectSources[272], 0x00000000, 0x00000081, 0, 0, 0, 10, 30 },
	{ &gObjectSources[273], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[274], 0x00000000, 0x00000001, 0, 0, 0, 8, 48 },
	{ &gObjectSources[275], 0x00000000, 0x00000081, 0, 0, 0, 13, 0 },
	{ &gObjectSources[276], 0x00000000, 0x00006003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[277], 0x00000000, 0x00000023, 0, 1, 20, 9, 1 },
	{ &gObjectSources[278], 0x00000000, 0x00000001, 0, 1, 20, 9, 2 },
	{ &gObjectSources[279], 0x00000000, 0x00000081, 0, 0, 0, 10, 40 },
	{ &gObjectSources[280], 0x00000000, 0x00000181, 0, 0, 0, 17, 0 },
	{ &gObjectSources[281], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[282], 0x00000000, 0x00000081, 0, 1, 15, 8, 16 },
	{ &gObjectSources[283], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[284], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[285], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[286], 0x00000000, 0x00000001, 0, 1, 15, 8, 17 },
	{ &gObjectSources[287], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[288], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[289], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[290], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[291], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[292], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[293], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[294], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[295], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[296], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[297], 0x00000000, 0x00000021, 0, 0, 0, 22, 0 },
	{ &gObjectSources[298], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[299], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[300], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[301], 0x00000000, 0x00000001, 0, 0, 0, 5, 6 },
	{ &gObjectSources[302], 0x00000000, 0x00004001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[303], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[304], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[305], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[306], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[307], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[308], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[309], 0x00000000, 0x00000001, 0, 1, 15, 8, 18 },
	{ &gObjectSources[310], 0x00000000, 0x00000001, 0, 1, 15, 8, 19 },
	{ &gObjectSources[311], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[312], 0x00000000, 0x00000001, 0, 0, 0, 20, 1 },
	{ &gObjectSources[313], 0x00000000, 0x00000001, 0, 0, 0, 20, 18 },
	{ &gObjectSources[314], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[315], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[316], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[317], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[318], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[319], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[320], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[321], 0x00000000, 0x00000001, 0, 0, 0, 7, 101 },
	{ &gObjectSources[322], 0x00000000, 0x00000001, 0, 0, 0, 7, 102 },
	{ &gObjectSources[323], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[324], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[325], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[326], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[327], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[328], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[329], 0x00000000, 0x00000001, 0, 0, 0, 5, 0 },
	{ &gObjectSources[330], 0x00000000, 0x00000001, 0, 0, 0, 5, 0 },
	{ &gObjectSources[331], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[332], 0x00000000, 0x00000081, 0, 1, 15, 8, 21 },
	{ &gObjectSources[333], 0x00000000, 0x00000001, 0, 1, 15, 22, 0 },
	{ &gObjectSources[334], 0x00000000, 0x00000003, 0, 1, 15, 22, 0 },
	{ &gObjectSources[335], 0x00000000, 0x00000001, 0, 1, 15, 22, 0 },
	{ &gObjectSources[336], 0x00000000, 0x00000001, 0, 1, 15, 22, 0 },
	{ &gObjectSources[337], 0x00000000, 0x00000001, 0, 1, 15, 22, 0 },
	{ &gObjectSources[338], 0x00000000, 0x00000001, 0, 1, 15, 22, 0 },
	{ &gObjectSources[339], 0x00000000, 0x00000001, 0, 1, 15, 22, 0 },
	{ &gObjectSources[340], 0x00000000, 0x00000001, 0, 1, 15, 22, 0 },
	{ &gObjectSources[341], 0x00000000, 0x00000081, 0, 1, 15, 8, 22 },
	{ &gObjectSources[342], 0x00000000, 0x00000081, 0, 0, 0, 10, 50 },
	{ &gObjectSources[343], 0x00000000, 0x00000001, 0, 0, 0, 10, 51 },
	{ &gObjectSources[344], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[345], 0x00000000, 0x00000081, 0, 0, 0, 10, 60 },
	{ &gObjectSources[346], 0x00000000, 0x00000001, 0, 1, 15, 8, 23 },
	{ &gObjectSources[347], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[348], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[349], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[350], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[351], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[352], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[353], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[354], 0x00000000, 0x00000181, 0, 0, 0, 18, 0 },
	{ &gObjectSources[355], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[356], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[357], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[358], 0x00000000, 0x00000001, 0, 0, 0, 20, 18 },
	{ &gObjectSources[359], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[360], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[361], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[362], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[363], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[364], 0x00000000, 0x00000001, 0, 0, 0, 10, 70 },
	{ &gObjectSources[286], 0x00000000, 0x00000001, 0, 1, 15, 8, 24 },
	{ &gObjectSources[286], 0x00000000, 0x00000001, 0, 1, 15, 8, 25 },
	{ &gObjectSources[286], 0x00000000, 0x00000001, 0, 1, 15, 8, 26 },
	{ &gObjectSources[286], 0x00000000, 0x00000001, 0, 1, 15, 8, 27 },
	{ &gObjectSources[286], 0x00000000, 0x00000001, 0, 1, 15, 8, 28 },
	{ &gObjectSources[286], 0x00000000, 0x00000001, 0, 1, 15, 8, 29 },
	{ &gObjectSources[286], 0x00000000, 0x00000001, 0, 1, 15, 8, 30 },
	{ &gObjectSources[286], 0x00000000, 0x00000001, 0, 1, 15, 8, 31 },
	{ &gObjectSources[286], 0x00000000, 0x00000001, 0, 1, 15, 8, 32 },
	{ &gObjectSources[374], 0x00000000, 0x00000001, 0, 0, 0, 8, 33 },
	{ &gObjectSources[375], 0x00000000, 0x00000001, 0, 0, 0, 8, 34 },
	{ &gObjectSources[376], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[377], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[378], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[379], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[380], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[381], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[382], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[383], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[384], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[385], 0x00000000, 0x00000009, 3, 0, 0, 21, 0 },
	{ &gObjectSources[386], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[387], 0x00000000, 0x00000001, 0, 1, 15, 8, 35 },
	{ &gObjectSources[388], 0x00000000, 0x00000001, 0, 1, 15, 8, 36 },
	{ &gObjectSources[389], 0x00000000, 0x00000001, 0, 1, 15, 8, 37 },
	{ &gObjectSources[390], 0x00000000, 0x00000001, 0, 1, 15, 8, 38 },
	{ &gObjectSources[391], 0x00000000, 0x00000001, 0, 1, 15, 8, 39 },
	{ &gObjectSources[392], 0x00000000, 0x00000001, 0, 1, 15, 8, 40 },
	{ &gObjectSources[393], 0x00000000, 0x00000001, 0, 1, 15, 8, 41 },
	{ &gObjectSources[393], 0x00000000, 0x00000001, 0, 1, 15, 8, 42 },
	{ &gObjectSources[393], 0x00000000, 0x00000001, 0, 1, 15, 8, 43 },
	{ &gObjectSources[396], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[397], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[398], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[399], 0x00000000, 0x00000001, 0, 0, 0, 7, 300 },
	{ &gObjectSources[400], 0x00000000, 0x00000001, 0, 0, 0, 20, 50 },
	{ &gObjectSources[401], 0x00000000, 0x00000001, 0, 0, 0, 20, 50 },
	{ &gObjectSources[402], 0x00000000, 0x00000001, 0, 0, 0, 20, 50 },
	{ &gObjectSources[403], 0x00000000, 0x00000001, 0, 0, 0, 0, 0 },
	{ &gObjectSources[404], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[405], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[406], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[407], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[408], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[409], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[410], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[411], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[412], 0x00000000, 0x00000001, 0, 1, 15, 8, 44 },
	{ &gObjectSources[413], 0x00000000, 0x00000001, 0, 1, 15, 8, 45 },
	{ &gObjectSources[414], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[415], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[416], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[417], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[418], 0x00000000, 0x00000001, 0, 1, 15, 8, 46 },
	{ &gObjectSources[419], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[420], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[421], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[422], 0x00000000, 0x00000001, 0, 1, 15, 8, 47 },
	{ &gObjectSources[423], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[424], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[425], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[426], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[427], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[428], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[429], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[430], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[431], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[432], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[433], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[434], 0x00000000, 0x00000003, 0, 0, 0, 3, 0 },
	{ &gObjectSources[435], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[436], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[437], 0x00000000, 0x00000001, 0, 1, 15, 8, 49 },
	{ &gObjectSources[438], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[439], 0x00000000, 0x00000001, 0, 0, 0, 5, 2 },
	{ &gObjectSources[440], 0x00000000, 0x00000001, 0, 0, 0, 5, 3 },
	{ &gObjectSources[441], 0x00000000, 0x00000001, 0, 0, 0, 5, 4 },
	{ &gObjectSources[442], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[443], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[444], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[445], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[446], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[447], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[448], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[449], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[450], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[451], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[452], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[453], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[454], 0x00000000, 0x00000001, 0, 0, 0, 20, 18 },
	{ &gObjectSources[455], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[456], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[457], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[458], 0x00000000, 0x00000001, 0, 1, 15, 8, 51 },
	{ &gObjectSources[459], 0x00000000, 0x00000001, 0, 1, 15, 8, 52 },
	{ &gObjectSources[460], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[461], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[462], 0x00000000, 0x00000001, 0, 1, 15, 8, 53 },
	{ &gObjectSources[462], 0x00000000, 0x00000001, 0, 1, 15, 8, 54 },
	{ &gObjectSources[464], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[465], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[466], 0x00000000, 0x00000081, 0, 0, 0, 6, 0 },
	{ &gObjectSources[467], 0x00000000, 0x00000081, 0, 0, 0, 10, 0 },
	{ &gObjectSources[468], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[469], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[470], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[471], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[472], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[473], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[474], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[475], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[476], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[477], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[478], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[479], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[480], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[481], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[482], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[483], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[484], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[485], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[486], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[487], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[488], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[489], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[490], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[491], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[492], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[493], 0x00000000, 0x00002001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[494], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[495], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[496], 0x00000000, 0x00000001, 0, 1, 15, 8, 55 },
	{ &gObjectSources[497], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[498], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[499], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[500], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[501], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[502], 0x00000000, 0x00000003, 0, 0, 0, 3, 0 },
	{ &gObjectSources[503], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[504], 0x00000000, 0x00000081, 0, 1, 15, 8, 56 },
	{ &gObjectSources[505], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[506], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[507], 0x00000000, 0x00000081, 0, 0, 0, 10, 100 },
	{ &gObjectSources[508], 0x00000000, 0x00000081, 0, 0, 0, 10, 80 },
	{ &gObjectSources[509], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[510], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[511], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[512], 0x00000000, 0x00004001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[513], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[514], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[515], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[516], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[517], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[518], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[519], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[520], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[521], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[522], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[523], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[524], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[525], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[526], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[527], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[528], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[529], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[530], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[531], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[532], 0x00000000, 0x00000001, 0, 1, 15, 8, 63 },
	{ &gObjectSources[533], 0x00000000, 0x00000001, 0, 1, 15, 8, 64 },
	{ &gObjectSources[534], 0x00000000, 0x00000001, 0, 1, 15, 8, 65 },
	{ &gObjectSources[535], 0x00000000, 0x00000001, 0, 1, 15, 8, 66 },
	{ &gObjectSources[536], 0x00000000, 0x00000001, 0, 1, 15, 8, 67 },
	{ &gObjectSources[537], 0x00000000, 0x00000001, 0, 1, 15, 8, 68 },
	{ &gObjectSources[538], 0x00000000, 0x00000001, 0, 1, 15, 8, 69 },
	{ &gObjectSources[539], 0x00000000, 0x00000001, 0, 1, 15, 8, 70 },
	{ &gObjectSources[540], 0x00000000, 0x00000001, 0, 1, 15, 8, 71 },
	{ &gObjectSources[541], 0x00000000, 0x00000001, 0, 1, 15, 8, 72 },
	{ &gObjectSources[542], 0x00000000, 0x00000009, 2, 1, 15, 8, 73 },
	{ &gObjectSources[543], 0x00000000, 0x00000001, 0, 1, 15, 8, 74 },
	{ &gObjectSources[544], 0x00000000, 0x00000001, 0, 1, 15, 8, 75 },
	{ &gObjectSources[545], 0x00000000, 0x00000001, 0, 1, 15, 8, 76 },
	{ &gObjectSources[546], 0x00000000, 0x00000001, 0, 1, 15, 8, 77 },
	{ &gObjectSources[547], 0x00000000, 0x00000001, 0, 1, 15, 8, 78 },
	{ &gObjectSources[548], 0x00000000, 0x00000001, 0, 1, 15, 8, 79 },
	{ &gObjectSources[549], 0x00000000, 0x00000001, 0, 1, 15, 8, 80 },
	{ &gObjectSources[550], 0x00000000, 0x00000001, 0, 1, 15, 8, 81 },
	{ &gObjectSources[551], 0x00000000, 0x00000001, 0, 1, 15, 8, 82 },
	{ &gObjectSources[552], 0x00000000, 0x00000001, 0, 1, 15, 8, 83 },
	{ &gObjectSources[553], 0x00000000, 0x00000001, 0, 1, 15, 8, 84 },
	{ &gObjectSources[554], 0x00000000, 0x00000001, 0, 1, 15, 8, 85 },
	{ &gObjectSources[555], 0x00000000, 0x00000001, 0, 1, 15, 8, 86 },
	{ &gObjectSources[556], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[557], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[558], 0x00000000, 0x00000001, 0, 1, 15, 8, 57 },
	{ &gObjectSources[559], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[560], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[561], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[562], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[563], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[564], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[565], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[566], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[567], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[568], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[569], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[570], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[571], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[572], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[573], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[574], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[575], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[576], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[577], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[578], 0x00000000, 0x00000001, 0, 1, 15, 8, 58 },
	{ &gObjectSources[579], 0x00000000, 0x00000001, 0, 1, 15, 8, 59 },
	{ &gObjectSources[580], 0x00000000, 0x00000001, 0, 1, 15, 8, 60 },
	{ &gObjectSources[581], 0x00000000, 0x00000001, 0, 1, 15, 8, 61 },
	{ &gObjectSources[582], 0x00000000, 0x00000001, 0, 1, 15, 8, 62 },
	{ &gObjectSources[583], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[584], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[585], 0x00000000, 0x00000001, 0, 0, 0, 20, 36 },
	{ &gObjectSources[586], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[587], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[588], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[589], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[590], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[591], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[592], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[593], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[594], 0x00000000, 0x00002401, 0, 0, 0, 22, 0 },
	{ &gObjectSources[595], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[596], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[597], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[598], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[599], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[600], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[601], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[602], 0x00000000, 0x00000001, 0, 0, 0, 20, 18 },
	{ &gObjectSources[603], 0x00000000, 0x00000001, 0, 0, 0, 20, 18 },
	{ &gObjectSources[604], 0x00000000, 0x00000001, 0, 0, 0, 20, 18 },
	{ &gObjectSources[605], 0x00000000, 0x00000001, 0, 0, 0, 20, 50 },
	{ &gObjectSources[606], 0x00000000, 0x00000081, 0, 0, 0, 10, 120 },
	{ &gObjectSources[607], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[608], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[609], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[610], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[611], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[612], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[613], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[614], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[615], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[616], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[617], 0x00000000, 0x00000001, 0, 0, 0, 0, 0 },
	{ &gObjectSources[618], 0x00000000, 0x00000001, 0, 0, 0, 0, 0 },
	{ &gObjectSources[619], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[620], 0x00000000, 0x00000081, 0, 0, 0, 10, 90 },
	{ &gObjectSources[621], 0x00000000, 0x00000081, 0, 0, 0, 10, 110 },
	{ &gObjectSources[622], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[623], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[624], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[625], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[626], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[627], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[628], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[629], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[630], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[631], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[632], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[633], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[634], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[635], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[636], 0x00000000, 0x00000001, 0, 1, 15, 8, 87 },
	{ &gObjectSources[637], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[638], 0x00000000, 0x00000001, 0, 1, 15, 8, 88 },
	{ &gObjectSources[639], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[636], 0x00000000, 0x00000001, 0, 1, 15, 8, 89 },
	{ &gObjectSources[636], 0x00000000, 0x00000001, 0, 1, 15, 8, 90 },
	{ &gObjectSources[642], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[643], 0x00000000, 0x00000001, 0, 1, 15, 8, 91 },
	{ &gObjectSources[644], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[645], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[646], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[647], 0x00000000, 0x00000001, 0, 1, 15, 8, 92 },
	{ &gObjectSources[647], 0x00000000, 0x00000001, 0, 1, 15, 8, 93 },
	{ &gObjectSources[647], 0x00000000, 0x00000001, 0, 1, 15, 8, 94 },
	{ &gObjectSources[650], 0x00000000, 0x00000001, 0, 1, 15, 8, 95 },
	{ &gObjectSources[651], 0x00000000, 0x00000001, 0, 1, 15, 8, 96 },
	{ &gObjectSources[652], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[653], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[654], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[655], 0x00000000, 0x00000001, 0, 1, 15, 8, 98 },
	{ &gObjectSources[656], 0x00000000, 0x00000001, 0, 1, 15, 8, 99 },
	{ &gObjectSources[657], 0x00000000, 0x00000001, 0, 1, 15, 8, 100 },
	{ &gObjectSources[658], 0x00000000, 0x00000001, 0, 1, 15, 8, 101 },
	{ &gObjectSources[659], 0x00000000, 0x00000001, 0, 1, 15, 8, 102 },
	{ &gObjectSources[660], 0x00000000, 0x00000001, 0, 1, 15, 8, 103 },
	{ &gObjectSources[661], 0x00000000, 0x00000001, 0, 1, 15, 8, 104 },
	{ &gObjectSources[662], 0x00000000, 0x00000001, 0, 1, 15, 8, 105 },
	{ &gObjectSources[663], 0x00000000, 0x00000001, 0, 1, 15, 8, 106 },
	{ &gObjectSources[664], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[665], 0x00000000, 0x00000001, 0, 0, 0, 8, 107 },
	{ &gObjectSources[666], 0x00000000, 0x00000001, 0, 1, 15, 8, 108 },
	{ &gObjectSources[667], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[668], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[669], 0x00000000, 0x00000081, 0, 0, 0, 22, 8 },
	{ &gObjectSources[670], 0x00000000, 0x00000081, 0, 0, 0, 5, 8 },
	{ &gObjectSources[671], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[672], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[673], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[674], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[675], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[676], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[677], 0x00000000, 0x00000001, 0, 0, 0, 3, 0 },
	{ &gObjectSources[678], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[679], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[680], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[681], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[682], 0x00000000, 0x00000081, 0, 0, 0, 5, 8 },
	{ &gObjectSources[683], 0x00000000, 0x00000001, 0, 1, 15, 8, 109 },
	{ &gObjectSources[684], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[685], 0x00000000, 0x00000001, 0, 1, 15, 8, 110 },
	{ &gObjectSources[686], 0x00000000, 0x00000001, 0, 0, 0, 8, 111 },
	{ &gObjectSources[687], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[688], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[689], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[690], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[691], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[692], 0x00000000, 0x00000201, 0, 0, 0, 22, 0 },
	{ &gObjectSources[693], 0x00000000, 0x00000001, 0, 1, 15, 8, 112 },
	{ &gObjectSources[694], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[695], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[696], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[697], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[698], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[699], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[700], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[701], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[702], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[703], 0x00000000, 0x00000001, 0, 1, 15, 8, 97 },
	{ &gObjectSources[704], 0x00000000, 0x00000001, 0, 1, 15, 8, 113 },
	{ &gObjectSources[705], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[706], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[707], 0x00000000, 0x00000081, 0, 0, 0, 5, 19 },
	{ &gObjectSources[708], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[709], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[710], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[711], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[712], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[713], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[714], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[715], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[716], 0x00000000, 0x00000003, 0, 0, 0, 3, 0 },
	{ &gObjectSources[717], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[718], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[719], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[720], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[721], 0x00000000, 0x00001001, 0, 0, 0, 20, 36 },
	{ &gObjectSources[722], 0x00000000, 0x00000003, 0, 0, 0, 3, 0 },
	{ &gObjectSources[723], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[724], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[725], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[726], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[727], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[728], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[729], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[730], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[731], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[732], 0x00000000, 0x00000001, 0, 0, 0, 10, 0 },
	{ &gObjectSources[733], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[734], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[735], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[736], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[737], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[738], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[739], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[740], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[741], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[742], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[743], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[744], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[745], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[746], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[747], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[748], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[749], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[750], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[751], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[752], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[753], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[754], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[755], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[756], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[757], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[758], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[759], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[760], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[761], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[762], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[763], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[764], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[763], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[764], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[763], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[763], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[764], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[763], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[764], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[763], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[764], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[763], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[763], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[776], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[777], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[778], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[779], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[780], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[781], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[782], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[783], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[784], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[785], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[786], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[787], 0x00000000, 0x00000201, 0, 0, 0, 22, 0 },
	{ &gObjectSources[788], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[789], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[790], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[791], 0x00000000, 0x00000081, 0, 0, 0, 10, 0 },
	{ &gObjectSources[792], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[793], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[794], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[795], 0x00000000, 0x00000001, 0, 0, 0, 5, 9 },
	{ &gObjectSources[796], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[797], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[798], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[799], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[800], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[801], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[802], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[803], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[682], 0x00000000, 0x00000001, 0, 1, 15, 8, 114 },
	{ &gObjectSources[805], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[806], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[807], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[808], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[809], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[810], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[811], 0x00000000, 0x00000001, 0, 1, 15, 8, 115 },
	{ &gObjectSources[812], 0x00000000, 0x00000001, 0, 1, 15, 8, 116 },
	{ &gObjectSources[813], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[814], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[815], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[816], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[817], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[818], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[819], 0x00000000, 0x00000001, 0, 1, 15, 8, 117 },
	{ &gObjectSources[820], 0x00000000, 0x00000001, 0, 1, 15, 8, 118 },
	{ &gObjectSources[821], 0x00000000, 0x00000001, 0, 1, 15, 8, 119 },
	{ &gObjectSources[822], 0x00000000, 0x00000001, 0, 1, 15, 8, 120 },
	{ &gObjectSources[823], 0x00000000, 0x00000001, 0, 1, 15, 8, 121 },
	{ &gObjectSources[824], 0x00000000, 0x00000001, 0, 1, 15, 8, 122 },
	{ &gObjectSources[825], 0x00000000, 0x00000001, 0, 1, 15, 8, 123 },
	{ &gObjectSources[826], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[827], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[828], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[829], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[830], 0x00000000, 0x00000001, 0, 1, 15, 8, 124 },
	{ &gObjectSources[831], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[832], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[833], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[834], 0x00000000, 0x00000001, 0, 1, 15, 8, 125 },
	{ &gObjectSources[835], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[836], 0x00000000, 0x00000001, 0, 1, 15, 8, 126 },
	{ &gObjectSources[837], 0x00000000, 0x00000001, 0, 1, 15, 8, 127 },
	{ &gObjectSources[838], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[839], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[840], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[841], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[842], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[843], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[844], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[845], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[846], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[847], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[848], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[849], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[850], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[851], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[852], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[853], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[854], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[855], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[856], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[857], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[858], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[859], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[860], 0x00000000, 0x00000001, 0, 1, 15, 8, 128 },
	{ &gObjectSources[861], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[862], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[863], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[864], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[865], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[866], 0x00000000, 0x00000001, 0, 1, 15, 8, 130 },
	{ &gObjectSources[867], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[868], 0x00000000, 0x00000001, 0, 1, 15, 8, 129 },
	{ &gObjectSources[869], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[870], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[871], 0x00000000, 0x00000001, 0, 1, 15, 8, 131 },
	{ &gObjectSources[872], 0x00000000, 0x00000001, 0, 1, 15, 8, 132 },
	{ &gObjectSources[873], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[874], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[875], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[876], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[877], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[878], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[879], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[880], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[881], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[882], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[883], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[884], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[885], 0x00000000, 0x00000001, 0, 1, 15, 8, 133 },
	{ &gObjectSources[886], 0x00000000, 0x00000081, 0, 0, 0, 15, 0 },
	{ &gObjectSources[887], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[888], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[889], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[890], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[891], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[892], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[893], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[894], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[895], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[896], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[897], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[898], 0x00000000, 0x00000001, 0, 0, 0, 20, 18 },
	{ &gObjectSources[899], 0x00000000, 0x00000001, 0, 0, 0, 20, 18 },
	{ &gObjectSources[900], 0x00000000, 0x00000181, 0, 0, 0, 19, 0 },
	{ &gObjectSources[901], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[902], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[903], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[904], 0x00000000, 0x00000001, 0, 0, 0, 20, 18 },
	{ &gObjectSources[905], 0x00000000, 0x00000001, 0, 0, 0, 20, 18 },
	{ &gObjectSources[906], 0x00000000, 0x00000001, 0, 0, 0, 20, 34 },
	{ &gObjectSources[907], 0x00000000, 0x00000801, 0, 0, 0, 22, 0 },
	{ &gObjectSources[908], 0x00000000, 0x00000401, 0, 0, 0, 22, 0 },
	{ &gObjectSources[909], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[467], 0x00000000, 0x00000081, 0, 1, 15, 8, 134 },
	{ &gObjectSources[911], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[912], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[913], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[914], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[915], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[916], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[917], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[918], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[919], 0x00000000, 0x00000001, 0, 1, 15, 8, 135 },
	{ &gObjectSources[920], 0x00000000, 0x00000001, 0, 1, 15, 8, 136 },
	{ &gObjectSources[921], 0x00000000, 0x00000001, 0, 1, 15, 8, 137 },
	{ &gObjectSources[922], 0x00000000, 0x00000001, 0, 1, 15, 8, 138 },
	{ &gObjectSources[923], 0x00000000, 0x00000001, 0, 1, 15, 8, 139 },
	{ &gObjectSources[924], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[925], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[926], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[927], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[928], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[929], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[930], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[931], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[932], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[933], 0x00000000, 0x00000001, 0, 1, 15, 8, 140 },
	{ &gObjectSources[934], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[935], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[936], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[937], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[938], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[939], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[940], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[941], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[942], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[943], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[944], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[945], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[946], 0x00000000, 0x00000003, 0, 0, 0, 3, 0 },
	{ &gObjectSources[947], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[948], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[949], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[950], 0x00000000, 0x00000001, 0, 1, 15, 8, 141 },
	{ &gObjectSources[950], 0x00000000, 0x00000001, 0, 1, 15, 8, 142 },
	{ &gObjectSources[950], 0x00000000, 0x00000001, 0, 1, 15, 8, 143 },
	{ &gObjectSources[950], 0x00000000, 0x00000001, 0, 1, 15, 8, 144 },
	{ &gObjectSources[950], 0x00000000, 0x00000001, 0, 1, 15, 8, 145 },
	{ &gObjectSources[955], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[956], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[957], 0x00000000, 0x00000001, 0, 0, 0, 5, 12 },
	{ &gObjectSources[958], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[959], 0x00000000, 0x00000001, 0, 0, 0, 5, 8 },
	{ &gObjectSources[343], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[961], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[962], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[963], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[964], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[965], 0x00000000, 0x00000001, 0, 0, 0, 20, 18 },
	{ &gObjectSources[966], 0x00000000, 0x00000001, 0, 0, 0, 20, 18 },
	{ &gObjectSources[967], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[968], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[969], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[970], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[971], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[972], 0x00000000, 0x00000801, 0, 0, 0, 22, 0 },
	{ &gObjectSources[973], 0x00000000, 0x00000001, 0, 0, 0, 5, 11 },
	{ &gObjectSources[974], 0x00000000, 0x00000001, 0, 0, 0, 5, 11 },
	{ &gObjectSources[975], 0x00000000, 0x00000001, 0, 0, 0, 5, 11 },
	{ &gObjectSources[976], 0x00000000, 0x00000001, 0, 0, 0, 5, 11 },
	{ &gObjectSources[977], 0x00000000, 0x00000001, 0, 0, 0, 5, 11 },
	{ &gObjectSources[978], 0x00000000, 0x00000001, 0, 0, 0, 5, 11 },
	{ &gObjectSources[979], 0x00000000, 0x00000001, 0, 0, 0, 5, 11 },
	{ &gObjectSources[980], 0x00000000, 0x00000001, 0, 0, 0, 5, 11 },
	{ &gObjectSources[981], 0x00000000, 0x00000001, 0, 0, 0, 5, 11 },
	{ &gObjectSources[982], 0x00000000, 0x00000001, 0, 1, 15, 8, 146 },
	{ &gObjectSources[983], 0x00000000, 0x00000001, 0, 1, 15, 8, 147 },
	{ &gObjectSources[984], 0x00000000, 0x00000001, 0, 1, 15, 8, 148 },
	{ &gObjectSources[985], 0x00000000, 0x00000001, 0, 1, 15, 8, 149 },
	{ &gObjectSources[986], 0x00000000, 0x00000001, 0, 1, 15, 8, 150 },
	{ &gObjectSources[987], 0x00000000, 0x00000001, 0, 1, 15, 8, 151 },
	{ &gObjectSources[988], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[989], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[990], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[991], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[992], 0x00000000, 0x00000001, 0, 0, 0, 5, 13 },
	{ &gObjectSources[993], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[994], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[995], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[996], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[997], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[998], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[999], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1000], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1001], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1002], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1003], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1004], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1005], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1006], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1007], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1008], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1009], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1010], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1011], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1012], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1013], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1014], 0x00000000, 0x00000003, 0, 0, 0, 3, 0 },
	{ &gObjectSources[1015], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1016], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1017], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1018], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1019], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1020], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1021], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1022], 0x00000000, 0x00000001, 0, 0, 0, 5, 2 },
	{ &gObjectSources[1023], 0x00000000, 0x00000001, 0, 0, 0, 5, 3 },
	{ &gObjectSources[1024], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1025], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1026], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1027], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1028], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1029], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1030], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1031], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1032], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1033], 0x00000000, 0x00000801, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1034], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1035], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1036], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1037], 0x00000000, 0x00000001, 0, 0, 0, 5, 16 },
	{ &gObjectSources[1038], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1039], 0x00000000, 0x00000001, 0, 0, 0, 5, 2 },
	{ &gObjectSources[1040], 0x00000000, 0x00000001, 0, 0, 0, 5, 3 },
	{ &gObjectSources[1041], 0x00000000, 0x00000001, 0, 0, 0, 5, 2 },
	{ &gObjectSources[1042], 0x00000000, 0x00000001, 0, 0, 0, 5, 3 },
	{ &gObjectSources[1043], 0x00000000, 0x00000001, 0, 0, 0, 5, 2 },
	{ &gObjectSources[1044], 0x00000000, 0x00000001, 0, 0, 0, 5, 3 },
	{ &gObjectSources[647], 0x00000000, 0x00000001, 0, 1, 15, 8, 152 },
	{ &gObjectSources[1046], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1047], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1048], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1049], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1050], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1051], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1052], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1053], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1054], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1055], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1056], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1057], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1058], 0x00000000, 0x00000001, 0, 1, 15, 8, 153 },
	{ &gObjectSources[1059], 0x00000000, 0x00000001, 0, 0, 0, 17, 0 },
	{ &gObjectSources[1060], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1061], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1062], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1063], 0x00000000, 0x00000001, 0, 1, 15, 8, 154 },
	{ &gObjectSources[1064], 0x00000000, 0x00000001, 0, 1, 15, 8, 155 },
	{ &gObjectSources[1065], 0x00000000, 0x00000081, 0, 0, 0, 5, 0 },
	{ &gObjectSources[1066], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1067], 0x00000000, 0x00000001, 0, 0, 0, 5, 14 },
	{ &gObjectSources[1068], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1069], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1070], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1071], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1072], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1073], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1074], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1075], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1076], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1077], 0x00000000, 0x00000001, 0, 0, 0, 5, 2 },
	{ &gObjectSources[1078], 0x00000000, 0x00000001, 0, 0, 0, 5, 3 },
	{ &gObjectSources[1079], 0x00000000, 0x00000001, 0, 0, 0, 5, 4 },
	{ &gObjectSources[1080], 0x00000000, 0x00000003, 0, 0, 0, 5, 18 },
	{ &gObjectSources[1081], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1082], 0x00000000, 0x00000001, 0, 0, 0, 5, 15 },
	{ &gObjectSources[1083], 0x00000000, 0x00004001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1084], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1085], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1086], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1087], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1088], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1089], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1090], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1091], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1092], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1093], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1094], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1095], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1096], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1097], 0x00000000, 0x00000001, 0, 0, 0, 0, 0 },
	{ &gObjectSources[1098], 0x00000000, 0x00000001, 0, 1, 15, 8, 156 },
	{ &gObjectSources[1099], 0x00000000, 0x00000001, 0, 1, 15, 8, 157 },
	{ &gObjectSources[1100], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1101], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1102], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1103], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1104], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1105], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[274], 0x00000000, 0x00000001, 0, 0, 0, 5, 0 },
	{ &gObjectSources[1107], 0x00000000, 0x00004001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1108], 0x00000000, 0x00004001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1109], 0x00000000, 0x00004001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1110], 0x00000000, 0x00004001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1111], 0x00000000, 0x00001001, 0, 0, 0, 20, 50 },
	{ &gObjectSources[1112], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1113], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1114], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1115], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1116], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1117], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1118], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1119], 0x00000000, 0x00000801, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1120], 0x00000000, 0x00000801, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1121], 0x00000000, 0x00000801, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1122], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1123], 0x00000000, 0x00000801, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1124], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1125], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1126], 0x00000000, 0x00000081, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1127], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1128], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1129], 0x00000000, 0x00000001, 0, 0, 0, 8, 158 },
	{ &gObjectSources[1130], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1131], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1132], 0x00000000, 0x00000003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1133], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1134], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1135], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1136], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1137], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1138], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1139], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1140], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1141], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1142], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1143], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1144], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1145], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1146], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1147], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1148], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1149], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1150], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1151], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1152], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1153], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1154], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1155], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1156], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1157], 0x00000000, 0x00000081, 0, 0, 0, 10, 90 },
	{ &gObjectSources[1158], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1159], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1160], 0x00000000, 0x00004001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1161], 0x00000000, 0x00000001, 0, 0, 0, 22, 0 },
};

XObjSource gTestShortplayTitleSource   = { sTestShortplayTitleResourcePath };
XObjSource gChargeFx01Source           = { sChargeFx01ResourcePath };
XObjSource gCbPropellerLv2Source       = { sCbPropellerLv2ResourcePath };
XObjSource gCbCannonLv2Source          = { sCbCannonLv2ResourcePath };
XObjSource gCbRadarSource              = { sCbRadarResourcePath };
XObjSource gMaruBatuSource             = { sMaruBatuResourcePath };
XObjSource gExclamationSource          = { sExclamationResourcePath };
XObjSource gKisekaePartsKeathSource    = { sKisekaePartsKeathResourcePath };
XObjSource gKisekaePartsKaeruSource    = { sKisekaePartsKaeruResourcePath };
XObjSource gKisekaePartsTaoSource10021 = { sKisekaePartsTaoResourcePath };
XObjSource gKisekaePartsKegaSource     = { sKisekaePartsKegaResourcePath };
XObjSource gKisekaePartsTaoSource10023 = { sKisekaePartsTaoResourcePath };
XObjSource gKisekaePartsObakeSource    = { sKisekaePartsObakeResourcePath };
XObjSource gKisekaePartsPajamaSource   = { sKisekaePartsPajamaResourcePath };
XObjSource gKisekaePartsSuperSource    = { sKisekaePartsSuperResourcePath };
XObjSource gKisekaePartsPajama01Source = { sKisekaePartsPajama01ResourcePath };
XObjSource gKisekaePartsPajama02Source = { sKisekaePartsPajama02ResourcePath };
XObjSource gTitleIconIconBbSource      = { sTitleIconIconBbResourcePath };
XObjSource gTitleIconIconArmySource    = { sTitleIconIconArmyResourcePath };
XObjSource gTitleIconIconFafaSource    = { sTitleIconIconFafaResourcePath };
XObjSource gTitleIconIconFunkySource   = { sTitleIconIconFunkyResourcePath };
XObjSource gTitleIconIconHockSource    = { sTitleIconIconHockResourcePath };
XObjSource gTitleIconIconKeathSource   = { sTitleIconIconKeathResourcePath };
XObjSource gTitleIconIconPeetsSource   = { sTitleIconIconPeetsResourcePath };
XObjSource gTitleIconIconSunmidaSource = { sTitleIconIconSunmidaResourcePath };
XObjSource gTitleIconIconTrexSource    = { sTitleIconIconTrexResourcePath };
XObjSource gTitleIconIconZobinSource   = { sTitleIconIconZobinResourcePath };
XObjSource gTitleIconIconArmy2Source   = { sTitleIconIconArmy2ResourcePath };
#if !defined(VERSION_JP)
XObjSource gUsMaruBatuSource  = { "us_maru_batu" };
XObjSource gUsMaruModelSource = { "us_maru_model" };
XObjSource gUsBatuModelSource = { "us_batu_model" };
#endif
XObjSource gTestWardrobeSource10029 = { sTestWardrobeResourcePath };
XObjSource gTestWardrobeSource10030 = { sTestWardrobeResourcePath };

// Extended object IDs start at 10000 and index this table in GetObjData.
UnkObjData objTableEx[] = {
	{ &gTitleIconIconBbSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconArmySource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconFafaSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconFunkySource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconHockSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconKeathSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconPeetsSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconSunmidaSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconTrexSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconZobinSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconArmy2Source, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTestShortplayTitleSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[15], 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gChargeFx01Source, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gCbPropellerLv2Source, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gCbCannonLv2Source, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gCbRadarSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gMaruBatuSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gExclamationSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsKeathSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsKaeruSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsTaoSource10021, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsKegaSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsTaoSource10023, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsObakeSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsPajamaSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsSuperSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsPajama01Source, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsPajama02Source, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTestWardrobeSource10029, 0, 0x401, 0, 0, 0, 22, 0 },
	{ &gTestWardrobeSource10030, 0, 0x401, 0, 0, 0, 22, 0 },
	{ &gObjectSources[312], 0, 0x1, 0, 0, 0, 20, 1 },
	{ &gObjectSources[313], 0, 0x1, 0, 0, 0, 20, 50 },
#if !defined(VERSION_JP)
	{ &gUsMaruBatuSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gUsMaruModelSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gUsBatuModelSource, 0, 0x1, 0, 0, 0, 22, 0 },
#endif
};

// Object IDs from 10000 reuse these collision lists. A negative entry has no list.
s32 gObjectCollisionIdRemap[] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 19, 19, 312, 313,
#if !defined(VERSION_JP)
	-1, -1, -1,
#endif
};

UnkStageObj spawnTable[] = {
	// Title icons and demo.
	{ 10000, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10001, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10002, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10003, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10004, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10005, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10006, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10007, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10008, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10009, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10010, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
#if defined(VERSION_PAL)
	{ 10011, { 0, 0, 0 }, { 0, 0, 0 }, { 104, 104, 104 }, 0, 0, 0, 0x3 },
#else
	{ 10011, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
#endif

	// Menu and message cursors.
	{ 10012, { 0, -1500, 0 }, { 0, 0, 0 }, { 600, 600, 600 }, 0, 0, 1, 0x3 },
	{ 10012, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 1, 0x3 },

	// Player and equipment.
	{ 0, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x2301 },
	{ 17, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x10003 },
	{ 10013, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10014, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10015, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10016, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10017, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10018, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },

	{ 105, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1000011 },
	{ 105, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x2000011 },
	{ 105, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1000001 },
	{ 105, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x2000001 },

	// Costume parts.
	{ 10019, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10020, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10021, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10022, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10023, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10024, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10025, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10026, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10027, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10028, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },

	{ 399, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 1, 0x3 },
	{ 444, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1000003 },
	{ 260, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x2000007 },
	{ 261, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x2000007 },
	{ 321, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x2000007 },
	{ 322, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x2000007 },
	{ 428, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1000003 },
	{ 435, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1000003 },
#if !defined(VERSION_JP)
	{ 10033, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10034, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1 },
	{ 10035, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1 },
#endif
};
}

/**
 * @note Address: 0x8006EDC8
 * @note Size: 0x14
 */
extern "C" UnkStageObj* GetSpawnData(s32 index)
{
	return &spawnTable[index];
}

/**
 * @note Address: 0x8006EDDC
 * @note Size: 0x3C
 */
extern "C" UnkObjData* GetObjData(const CoordInfo* info)
{
	s32 index = info->mPlacement->mObjectId;
	if (index >= 10000)
		return &objTableEx[index - 10000];
	return &objTable[index];
}

/**
 * @note Address: 0x8006EE18
 * @note Size: 0xC
 */
extern "C" XObjSource* GetObjectSources()
{
	return gObjectSources;
}

#if defined(VERSION_PAL)
#define STAGE(file, message, flags) { file, file, message, flags }
#else
#define STAGE(file, message, flags) { file, message, flags }
#endif

StageDesc stageTable[] = {
	STAGE("stage00", 0, 5),
	STAGE("stage01", 1, 5),
	STAGE("stage02", 2, 5),
	STAGE("stage03", 3, 5),
	STAGE("stage04", 4, 5),
	STAGE("stage05", 5, 4),
	STAGE("stage06", 6, 5),
	STAGE("stage07", 7, 5),
	STAGE("stage08", 8, 5),
	STAGE("stage09", 9, 5),
	STAGE("stage10", 10, 1),
	STAGE("stage11", 11, 5),
	STAGE("stage12", 12, 5),
	STAGE("stage13", 13, 5),
	STAGE("stage14", 14, 5),
	STAGE("stage15", 15, 5),
	STAGE("stage16", 16, 5),
	STAGE("stage17", 17, 5),
	STAGE("stage18", 18, 5),
	STAGE("stage19", 19, 5),
	STAGE("stage20", 20, 5),
	STAGE("stage21", 21, 5),
	STAGE("stage22", 22, 5),
	STAGE("stage23", 23, 5),
	STAGE("stage24", 24, 5),
	STAGE("stage25", 25, 5),
	STAGE("stage26", 26, 5),
	STAGE("stage27", 27, 5),
#if defined(VERSION_PAL)
	{ "stage28", "stage07", 28, 5 },
	{ "stage29", "stage07", 29, 5 },
#else
	STAGE("stage28", 28, 5),
	STAGE("stage29", 29, 5),
#endif
	STAGE(NULL, -1, 0),

	// FindStageIndexByName assigns these stages IDs starting at 100.
	STAGE("shun", -1, 5),
	STAGE("hori", -1, 5),
	STAGE("ahk", -1, 5),
	STAGE("junko", -1, 5),
	STAGE("sayoko", -1, 5),
	STAGE("mory", -1, 5),
	STAGE("ryosuke", -1, 5),
	STAGE("takanabe", -1, 5),
	STAGE("mariko", -1, 5),
	STAGE(NULL, 0, 0),
};

#undef STAGE
#else
// the Wii's table also names each stage's sound bank and the bank's first sound id
StageDesc stageTable[] = {
	{ "stage00", "stage00", NULL, -1, 0, 5 },
	{ "stage01", "stage01", "sfx_kitchen.ssm", 0x245, 1, 5 },
	{ "stage02", "stage02", "sfx_stage02.ssm", 0x22d, 2, 5 },
	{ "stage03", "stage03", "sfx_souko.ssm", 0x24d, 3, 5 },
	{ "stage04", "stage04", "sfx_jennyroom.ssm", 0x23e, 4, 5 },
	{ "stage05", "stage05", "sfx_stage05.ssm", 0x252, 5, 4 },
	{ "stage06", "stage06", "sfx_papamama.ssm", 0x237, 6, 5 },
	{ "stage07", "stage07", "sfx_stage07.ssm", 0x233, 7, 5 },
	{ "stage08", "stage08", NULL, -1, 8, 5 },
	{ "stage09", "stage09", "sfx_uraniwa.ssm", 0x249, 9, 5 },
	{ "stage10", "stage10", NULL, -1, 10, 1 },
	{ "stage11", "stage11", "sfx_pipe.ssm", 0x259, 11, 5 },
	{ "stage12", "stage12", NULL, -1, 12, 5 },
	{ "stage13", "stage13", "sfx_manual.ssm", -1, 13, 5 },
	{ "stage14", "stage14", "sfx_stage07.ssm", -1, 14, 5 },
	{ "stage15", "stage15", NULL, -1, 15, 5 },
	{ "stage16", "stage16", "sfx_ufo.ssm", 0x25f, 16, 5 },
	{ "stage17", "stage17", NULL, -1, 17, 5 },
	{ "stage18", "stage18", "sfx_ufo.ssm", 0x237, 18, 5 },
	{ "stage19", "stage19", "sfx_stage07.ssm", -1, 19, 5 },
	{ "stage20", "stage20", NULL, -1, 20, 5 },
	{ "stage21", "stage21", NULL, -1, 21, 5 },
	{ "stage22", "stage22", "sfx_martial.ssm", -1, 22, 5 },
	{ "stage23", "stage23", NULL, -1, 23, 5 },
	{ "stage24", "stage24", NULL, -1, 24, 5 },
	{ "stage25", "stage25", NULL, -1, 25, 5 },
	{ "stage26", "stage26", NULL, -1, 26, 5 },
	{ "stage27", "stage27", NULL, -1, 27, 5 },
	{ "stage28", "stage07", NULL, -1, 28, 5 },
	{ "stage29", "stage07", NULL, -1, 29, 5 },
	{ NULL, NULL, NULL, -1, -1, 0 },
	{ "shun", "shun", NULL, -1, -1, 5 },
	{ "hori", "hori", "sfx_manual.ssm", -1, -1, 5 },
	{ "ahk", "ahk", NULL, -1, -1, 5 },
	{ "junko", "junko", NULL, -1, -1, 5 },
	{ "sayoko", "sayoko", NULL, -1, -1, 5 },
	{ "mory", "mory", NULL, -1, -1, 5 },
	{ "ryosuke", "ryosuke", NULL, -1, -1, 5 },
	{ "takanabe", "takanabe", NULL, -1, -1, 5 },
	{ "mariko", "mariko", NULL, -1, -1, 5 },
	{ NULL, NULL, NULL, 0, 0, 0 },
};

// the Wii keeps the object, spawn and placement tables in its own data; their
// accessors are the same as the GameCube file's
extern UnkStageObj gStageObjectPlacements[];
extern UnkStageObj spawnTable[];
extern UnkObjData objTable[];
extern UnkObjData objTableEx[];

// placement fixup lists: id -1 ends each table
struct PlacementChain {
	u8 _00[8];
	u32 mId;
	u8 _0C[4];
	s32* mChain;
	s32 mIndex;
};

extern PlacementChain lbl_802F0138[];
extern PlacementChain lbl_802FDB30[];

/**
 * @note Address: 0x80092EC8
 * @note Size: 0xBC
 */
extern "C" void fn_80092EC8(void)
{
	PlacementChain* entry = lbl_802F0138;
	for (;;) {
		if (entry->mId == -1)
			break;
		entry->mIndex = 0;
		if (entry->mChain != NULL)
			while (entry->mChain[entry->mIndex] != 0)
				entry->mIndex++;
		entry++;
	}
	entry = lbl_802FDB30;
	for (;;) {
		if (entry->mId == -1)
			break;
		entry->mIndex = 0;
		if (entry->mChain != NULL)
			while (entry->mChain[entry->mIndex] != 0)
				entry->mIndex++;
		entry++;
	}
}

/**
 * @note Address: 0x80092F84
 * @note Size: 0x3C
 */
extern "C" UnkObjData* GetObjData(const CoordInfo* info)
{
	s32 index = info->mPlacement->mObjectId;
	if (index >= 10000)
		return &objTableEx[index - 10000];
	return &objTable[index];
}

/**
 * @note Address: 0x80092FC0
 * @note Size: 0x14
 */
extern "C" UnkStageObj* GetSpawnData(s32 index)
{
	return &spawnTable[index];
}

/**
 * @note Address: 0x80092FD4
 * @note Size: 0x14
 */
extern "C" UnkStageObj* GetStageObjectPlacementByIndex(s32 index)
{
	return &gStageObjectPlacements[index];
}

extern s32 gItemIconTextureIndices[];

/**
 * @note Address: 0x80092FE8
 * @note Size: 0x24
 */
extern "C" s32 GetItemIconTextureIndex(s32 index)
{
	if (index < 0)
		return -1;
	return gItemIconTextureIndices[index];
}

// the object archives are ResourceFile slots from a shared pool, claimed as objects spawn
ResourceFile* gObjResourceFiles;
s32 gObjResourceFileCount;

/**
 * @note Address: 0x8009300C
 * @note Size: 0x12C
 */
void AttrInfo::Init(CoordInfo* info)
{
	UnkObjData* objData    = GetObjData(info);
	UnkStageObj* placement = info->mPlacement;
	ResourceFile* files    = gObjResourceFiles;
	mObjData               = objData;
	mPlacement             = placement;
	if (files == NULL)
		gObjResourceFiles = new (4) ResourceFile[0x530];
	if ((mObjData ? mObjData->mSource : NULL) == NULL)
		return;
	if ((mObjData ? mObjData->mSource : NULL) ? mObjData->mSource->mArchive : NULL)
		return;
	(mObjData ? mObjData->mSource : NULL)->mArchive = &gObjResourceFiles[gObjResourceFileCount++];
}
#endif

const char sStageDataPathFormat[]   = "bin/%s.bin";
const char sStageModulePathFormat[] = "rel/%s%s.rel";

/**
 * @note Address: 0x8006EE24
 * @note Size: 0x514
 */
extern "C" BOOL AdvanceStageLoad(Stage* stage, s32 index)
{
	switch (stage->mState) {
	case 0: {
#if defined(VERSION_JP)
		strcpy(stage->mDataPath, FormatToStaticBuffer(sStageDataPathFormat, stageTable[index].fileName));
#else
		strcpy(stage->mDataPath,
		       FormatToStaticBuffer("bin%s/%s.bin", gResourceDirectorySuffixByLanguage[gGame.mLanguage], stageTable[index].fileName));
#endif
#if defined(VERSION_R24J01)
		stage->mModulePath[0] = 0;
#else
#if defined(VERSION_PAL)
		// europe's table has the module's name apart from the data file's
		StageDesc* desc = &stageTable[index];
		strcpy(stage->mModulePath, FormatToStaticBuffer(sStageModulePathFormat, desc->moduleName, ""));
#else
		strcpy(stage->mModulePath, FormatToStaticBuffer(sStageModulePathFormat, stageTable[index].fileName, ""));
#endif
		if (!(gGame.mStatus.mFlags & 0x100000000ULL))
			stage->mModulePath[0] = 0; // an empty path skips the module load below
#endif
		++stage->mState;
	}
	// fall through, every step that's done moves on in the same frame
	case 1: {
		// read the stage's data file and resolve its sections
		if (!stage->mDataFile.ReadEx(stage->mDataPath, 0, 0))
			break;
		stage->mDataFile.CloseHandle();
		u8* data                      = (u8*)stage->mDataFile.mBuffer;
		const StageFileHeader* header = (const StageFileHeader*)data;
		stage->mEnvironment.mData     = data + ReadResourceOffset(header->environment);
		stage->mPlacements.mPlacement = (UnkStageObj*)(data + ReadResourceOffset(header->placements));
		stage->mScriptOffsets         = data + ReadResourceOffset(header->scriptOffsets);
		stage->mSetupScript           = data + ReadResourceOffset(header->setupScript);
		if (stage->mSetupScript == data) // a zero script offset means no script
			stage->mSetupScript = NULL;
		stage->mEntryScript = data + ReadResourceOffset(header->entryScript);
		if (stage->mEntryScript == data)
			stage->mEntryScript = NULL;
		stage->mBatteryDepletedScript = data + ReadResourceOffset(header->batteryDepletedScript);
		if (stage->mBatteryDepletedScript == data)
			stage->mBatteryDepletedScript = NULL;
		stage->mZButtonScript = data + ReadResourceOffset(header->zButtonScript);
		if (stage->mZButtonScript == data)
			stage->mZButtonScript = NULL;
		stage->mTimerExpiredScript = data + ReadResourceOffset(header->timerExpiredScript);
		if (stage->mTimerExpiredScript == data)
			stage->mTimerExpiredScript = NULL;
		stage->_198 = data + ReadResourceOffset(header->unknown20);
		if (stage->_198 == data)
			stage->_198 = NULL;
		stage->mEventObjectPairs  = (const StageEventObject*)(data + ReadResourceOffset(header->eventObjects));
		stage->mCameraAreaRecords = data + ReadResourceOffset(header->cameraAreas);
		if (stage->mCameraAreaRecords == data)
			stage->mCameraAreaRecords = NULL;
		stage->mCameraInfoRecords = data + ReadResourceOffset(header->cameraInfo);
		if (stage->mCameraInfoRecords == data)
			stage->mCameraInfoRecords = NULL;
		stage->mCameraAreaExRecords = data + ReadResourceOffset(header->cameraAreasEx);
		if (stage->mCameraAreaExRecords == data)
			stage->mCameraAreaExRecords = NULL;
		++stage->mState;
	}
	case 2: // load the stage's rel module when it has one
		if ((s8)stage->mModulePath[0] != 0) {
			if (!stage->mModule.Load(stage->mModulePath))
				break;
			++stage->mState;
		} else {
			++stage->mState;
		}
	case 3:
		++stage->mState;
	case 4: // the stage's sound bank
#if defined(VERSION_R24J01)
		if (stageTable[index].soundBank) {
			LoadWiiSoundBankByName(&gGame.mSound, stageTable[index].soundBank);
			++stage->mState;
		} else {
			stage->mState += 2;
		}
		break;
	case 5:
		// The Wii sound system clears this flag when the bank finishes loading.
		if (!gGame.mSound.mLoadPending)
			++stage->mState;
#else
		if (LoadGameSoundBank(&gGame.mSound, index))
			++stage->mState;
#endif
		break;
	}
#if defined(VERSION_R24J01)
	return stage->mState > 5;
#else
	return stage->mState > 4;
#endif
}

/**
 * @note Address: 0x8006F338
 * @note Size: 0x58
 */
extern "C" s32 CallStageModule(s32 function, s32 count, s32* args)
{
#if defined(VERSION_R24J01)
	return _prolog(function, count, args);
#else
	if (IsModuleLoaded(&gStageResources.mModule))
		return gStageResources.CallModule(function, count, args);
	return -1;
#endif
}

#if defined(VERSION_R24J01)
// Three Wii callers query this switch. The retail implementation always returns zero.
extern "C" s32 fn_800929EC()
{
	return 0;
}
#endif

/**
 * @note Address: 0x8006F390
 * @note Size: 0x1C
 */
extern "C" s32 GetStageCollectionFlagBase(s32 index)
{
	return ((s16*)(gStageResources.mEnvironment.mData + 0xA))[index];
}

/**
 * @note Address: 0x8006F3AC
 * @note Size: 0x10
 */
extern "C" CamAreaRecord* GetStageCameraAreaExRecords()
{
	return (CamAreaRecord*)gStageResources.mCameraAreaExRecords;
}

/**
 * @note Address: 0x8006F3BC
 * @note Size: 0x10
 */
extern "C" CamInfoData* GetStageCameraInfoRecords()
{
	return (CamInfoData*)gStageResources.mCameraInfoRecords;
}

/**
 * @note Address: 0x8006F3CC
 * @note Size: 0x10
 */
extern "C" CamAreaRecord* GetStageCameraAreaRecords()
{
	return (CamAreaRecord*)gStageResources.mCameraAreaRecords;
}

/**
 * @note Address: 0x8006F3DC
 * @note Size: 0x10
 */
extern "C" const StageEventObject* GetStageEventObjectPairs()
{
	return gStageResources.mEventObjectPairs;
}

/**
 * @note Address: 0x8006F3EC
 * @note Size: 0x10
 */
extern "C" u8* GetStageTimerExpiredScript()
{
	return gStageResources.mTimerExpiredScript;
}

/**
 * @note Address: 0x8006F3FC
 * @note Size: 0x10
 */
extern "C" u8* GetStageZButtonScript()
{
	return gStageResources.mZButtonScript;
}

/**
 * @note Address: 0x8006F40C
 * @note Size: 0x10
 */
extern "C" u8* GetStageBatteryDepletedScript()
{
	return gStageResources.mBatteryDepletedScript;
}

/**
 * @note Address: 0x8006F41C
 * @note Size: 0x10
 */
extern "C" u8* GetStageEntryScript()
{
	return gStageResources.mEntryScript;
}

/**
 * @note Address: 0x8006F42C
 * @note Size: 0x10
 */
extern "C" u8* GetStageSetupScript()
{
	return gStageResources.mSetupScript;
}

/**
 * @note Address: 0x8006F43C
 * @note Size: 0x54
 */
extern "C" u8* GetStageScriptByIndex(s32 index)
{
	u32 offset = ReadResourceOffset(gStageResources.mScriptOffsets + index * 4);
	return offset == 0 ? NULL : (u8*)gStageResources.mDataFile.mBuffer + offset;
}

/**
 * @note Address: 0x8006F490
 * @note Size: 0x10
 */
extern "C" u8* GetStageDataBuffer()
{
	return (u8*)gStageResources.mDataFile.mBuffer;
}

/**
 * @note Address: 0x8006F4A0
 * @note Size: 0x18
 */
extern "C" UnkStageObj* GetStagePlacementByIndex(s32 index)
{
	return gStageResources.mPlacements.mPlacement + index;
}

/**
 * @note Address: 0x8006F4B8
 * @note Size: 0x10
 */
extern "C" EnvInfo* GetStageEnvironment()
{
	return &gStageResources.mEnvironment;
}

/**
 * @note Address: 0x8006F4C8
 * @note Size: 0x1C
 */
extern "C" BOOL IsStageDataLoaded()
{
	return gStageResources.mDataFile.mBuffer != NULL;
}

/**
 * @note Address: 0x8006F4E4
 * @note Size: 0x60
 */
extern "C" void ApplyStageClearColor()
{
	// the environment record's first word is the VI background colour
#if defined(VERSION_R24J01)
	// the by-value GXColor argument materializes as byte copies from the unaligned
	// record; GC/3.0a5.2 emits the same lbz/stb sequence retail has at 0x80092858
	GXSetCopyClear(*(GXColor*)gStageResources.mEnvironment.mData, 0xFFFFFF);
#else
	GXColor color                  = *(GXColor*)gStageResources.mEnvironment.mData;
	*(GXColor*)(HSD_VIData + 0x48) = color; // current.vi.bg_color
#endif
	gStageResources.mModule.Print();
}

/**
 * @note Address: 0x8006F544
 * @note Size: 0x90
 */
extern "C" void UnloadStageDataAndModule()
{
#if defined(VERSION_R24J01)
	_prolog(6, 0, NULL);
#else
	if (IsModuleLoaded(&gStageResources.mModule))
		gStageResources.CallModule(6, 0, NULL); // slot 6 runs only before the module unloads
#endif
	gStageResources.mModule.Unload();
	CFile* file = &gStageResources.mDataFile;
	file->Close();
	gStageResources.mState = 0;
}

/**
 * @note Address: 0x8006F5D4
 * @note Size: 0x2C
 */
extern "C" BOOL LoadStageByIndex(s32 index)
{
	return AdvanceStageLoad(&gStageResources, index);
}

static inline s32 StageCount()
{
	s32 count = 0;
	while (stageTable[count].fileName != NULL)
		++count;
	return count;
}

// 100 and up are the stages after the table's null entry
static inline s32 StageIndex(s32 index)
{
	if (index < 100)
		return index;
	s32 count = StageCount();
	count += index;
	return count - 99;
}

/**
 * @note Address: 0x8006F600
 * @note Size: 0x54
 */
extern "C" u32 CheckStageFlag(s32 index, u32 mask)
{
	StageDesc* desc = &stageTable[StageIndex(index)];
	return mask & desc->flags;
}

/**
 * @note Address: 0x8006F654
 * @note Size: 0xE8
 */
extern "C" s32 FindStageIndexByName(const char* name)
{
	s32 index = 0;
	while (stageTable[index].fileName != NULL) { // the direct ids before the null entry
		if (strcmp(name, stageTable[index].fileName) == 0)
			return index;
		++index;
	}
	++index; // past the null entry the ids are 100-based
	while (stageTable[index].fileName != NULL) {
		if (strcmp(name, stageTable[index].fileName) == 0)
			return index - StageCount() + 99;
		++index;
	}
	return -1;
}

static inline const char* StageMessage(s32 index, s32 member)
{
	StageDesc* desc = &stageTable[StageIndex(index)];
	s32 message     = desc->message;
	if (message >= 0) {
		u8* data   = gGlobalScriptResources.mData;
		u8* record = data + ReadResourceOffset(data + GLOBAL_STAGE_NAMES) + message * 8;
		return (const char*)(data + ReadResourceOffset(record + member));
	}
	return NULL;
}

/**
 * @note Address: 0x8006F73C
 * @note Size: 0xD0
 */
extern "C" const char* GetStageDesc(s32 index)
{
	return StageMessage(index, 4);
}

/**
 * @note Address: 0x8006F80C
 * @note Size: 0xCC
 */
extern "C" const char* GetStageName(s32 index)
{
	return StageMessage(index, 0);
}

/**
 * @note Address: 0x8006F8D8
 * @note Size: 0x4C
 */
extern "C" const char* GetStageFileName(s32 index)
{
	index = StageIndex(index);
	return stageTable[index].fileName;
}

/**
 * @note Address: 0x8006F924
 * @note Size: 0x28
 */
extern "C" s32 GetStageNum()
{
	return StageCount();
}

#if defined(VERSION_R24J01)
extern "C" const char* GetStageNameOrFileName(s32 index)
{
	const char* name = GetStageName(index);
	if (name == NULL)
		name = GetStageFileName(index);
	return name;
}

extern "C" s32 GetStageFirstSoundId(s32 index)
{
	return stageTable[StageIndex(index)].firstSoundId;
}
#endif
