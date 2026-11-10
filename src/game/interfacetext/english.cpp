#include "Version.h"

extern "C" {
#if defined(VERSION_PAL)

extern const char sEnglishInterfaceText000[]      = "Do you want to display";
extern const char sEnglishInterfaceText001[]      = "the game in Progressive Mode?";
extern const char sEnglishInterfaceText002[]      = "%s%02XYes%s  %s%02XNo%s";
extern const char sEnglishInterfaceText003[]      = "Progressive Mode has";
extern const char sEnglishInterfaceText004[]      = "been turned on.";
extern const char gEmptyResourceDirectorySuffix[] = "";
extern const char sEnglishInterfaceText007[]      = "been turned off.";
extern const char sEnglishInterfaceText012[]      = "Store Item";
extern const char sEnglishInterfaceText013[]      = "Remove Suit";
extern const char sEnglishInterfaceText014[]      = "?";
extern const char sEnglishInterfaceText015[]      = "Zoom";
extern const char sEnglishInterfaceText016[]      = "Rumble";
extern const char sEnglishInterfaceText017[]      = "On";
extern const char sEnglishInterfaceText018[]      = "Off";
extern const char sEnglishInterfaceText019[]      = "Sound";
extern const char sEnglishInterfaceText020[]      = "Stereo";
extern const char sEnglishInterfaceText021[]      = "Mono";
extern const char sEnglishInterfaceText022[]      = "Back";
extern const char sEnglishInterfaceText023[]      = "Pause";
extern const char sEnglishInterfaceText024[]      = "Items";
extern const char sEnglishInterfaceText025[]      = "Controller";
extern const char sEnglishInterfaceText026[]      = "Status";
extern const char sEnglishInterfaceText027[]      = "Stickers";
extern const char sEnglishInterfaceText029[]      = "Action";
extern const char sEnglishInterfaceText030[]      = "Cancel";
extern const char sEnglishInterfaceText031[]      = "Rotate Camera";
extern const char sEnglishInterfaceText032[]      = "Equip Screen";
extern const char sEnglishInterfaceText033[]      = "Chibi-Vision";
extern const char sEnglishInterfaceText034[]      = "Pose";
extern const char sEnglishInterfaceText035[]      = "Center Camera";
extern const char sEnglishInterfaceText036[]      = "Map";
extern const char sEnglishInterfaceText038[]      = "Select";
extern const char sEnglishInterfaceText039[]      = "Control Chibi / Select";
extern const char sEnglishInterfaceText040[]      = "Chibi-Ranking";
extern const char sEnglishInterfaceText041[]      = "Chibi-Doors Completed";
extern const char sEnglishInterfaceText042[]      = "Happy Points";
extern const char sEnglishInterfaceText043[]      = "Moolah";
extern const char sEnglishInterfaceText044[]      = "Scrap";
extern const char sEnglishInterfaceText046[]      = "Chibi-PC Menu";
extern const char sEnglishInterfaceText047[]      = "Online Shop";
extern const char sEnglishInterfaceText057[]      = "End";
extern const char sEnglishInterfaceText060[]      = "Buy";
extern const char sEnglishInterfaceText062[]      = "Bought";
extern const char sEnglishInterfaceText063[]      = "Buy?";
extern const char sEnglishInterfaceText064[]      = "Yes";
extern const char sEnglishInterfaceText065[]      = "No";
extern const char sEnglishInterfaceText069[]      = "Recyclotron";
extern const char sEnglishInterfaceText070[]      = "Build";
extern const char sEnglishInterfaceText071[]      = "Exchanged";
extern const char sEnglishInterfaceText072[]      = "Exchange?";
extern const char sEnglishInterfaceText073[]      = "Scrap Trade";
extern const char sEnglishInterfaceText074[]      = "Exchange rate";
extern const char sEnglishInterfaceText075[]      = "Exchange how many?";
extern const char sEnglishInterfaceText076[]      = "You don't have enough Scrap.";
extern const char sEnglishInterfaceText077[]      = "It's ";
extern const char sEnglishInterfaceText078[]      = "OK?";
extern const char sEnglishInterfaceText079[]      = "\201\250";
extern const char sEnglishInterfaceText080[]      = "Build?";
extern const char sEnglishInterfaceText081[]      = "OK, stand by.";
extern const char sEnglishInterfaceText082[]      = "Exchange";
extern const char sEnglishInterfaceText084[]      = "The Disc Cover is open.\012Please close the Disc Cover\012to continue playing.";
extern const char sEnglishInterfaceText085[]      = "Please insert the Chibi-Robo Game Disc.";
extern const char sEnglishInterfaceText087[]      = "The Game Disc could not be read.\012Please read the Nintendo GameCube\012Instruction "
                                                    "Booklet for details.";
extern const char sEnglishInterfaceText088[]      = "Loading";
extern const char sEnglishInterfaceText090[]      = "Score";
extern const char sEnglishInterfaceText091[]      = "Time";
extern const char sEnglishInterfaceText092[]      = "Course";
extern const char sEnglishInterfaceText093[]      = "Enemy";
extern const char sEnglishInterfaceText094[]      = "New Game";
extern const char sEnglishInterfaceText095[]      = "Load Game";
extern const char sEnglishInterfaceText096[]      = "Options";
extern const char sEnglishInterfaceText097[]      = "Settings";
extern const char sEnglishInterfaceText098[]      = "Save";
extern const char sEnglishInterfaceText099[]      = "Empty";
extern const char sEnglishInterfaceText100[]      = "Yes  No";
extern const char sEnglishInterfaceText103[]      = "There is no Memory Card";
extern const char sEnglishInterfaceText104[]      = "in Slot A. Insert a Memory Card";
extern const char sEnglishInterfaceText105[]      = "or press the A Button to";
extern const char sEnglishInterfaceText106[]      = "continue without saving.";
extern const char sEnglishInterfaceText108[]      = "Wrong device in Slot A.";
extern const char sEnglishInterfaceText109[]      = "Please insert a different Memory Card or";
extern const char sEnglishInterfaceText110[]      = "press the A Button to continue";
extern const char sEnglishInterfaceText111[]      = "without saving.";
extern const char sEnglishInterfaceText113[]      = "The Memory Card in Slot A";
extern const char sEnglishInterfaceText114[]      = "cannot be used.";
extern const char sEnglishInterfaceText118[]      = "The Memory Card in Slot A is";
extern const char sEnglishInterfaceText119[]      = "corrupted and needs to be";
extern const char sEnglishInterfaceText120[]      = "formatted. Format now?";
extern const char sEnglishInterfaceText123[]      = "Formatting the Memory Card in Slot A";
extern const char sEnglishInterfaceText124[]      = "will erase all save data. Continue?";
extern const char sEnglishInterfaceText128[]      = "Formatting... Do not touch the";
extern const char sEnglishInterfaceText129[]      = "Memory Card in Slot A";
extern const char sEnglishInterfaceText130[]      = "or the POWER Button.";
extern const char sEnglishInterfaceText134[]      = "could not be formatted.";
extern const char sEnglishInterfaceText139[]      = "has been formatted.";
extern const char sEnglishInterfaceText143[]      = "Continue without saving?";
extern const char sEnglishInterfaceText148[]      = "To create new game data,";
extern const char sEnglishInterfaceText149[]      = "please select a file.";
extern const char sEnglishInterfaceText153[]      = "There is not";
extern const char sEnglishInterfaceText154[]      = "enough free space.";
extern const char sEnglishInterfaceText158[]      = "This data";
extern const char sEnglishInterfaceText159[]      = "is corrupted.";
extern const char sEnglishInterfaceText164[]      = "does not have enough space.";
extern const char sEnglishInterfaceText165[]      = "Chibi-Robo requires 1 file and";
extern const char sEnglishInterfaceText166[]      = "6 blocks to save.";
extern const char sEnglishInterfaceText168[]      = "Overwrite existing file?";
extern const char sEnglishInterfaceText173[]      = "To manage the contents of your Memory";
extern const char sEnglishInterfaceText174[]      = "Card, use the Memory Card screen.";
extern const char sEnglishInterfaceText178[]      = "Now saving...";
extern const char sEnglishInterfaceText179[]      = "Do not touch the Memory Card";
extern const char sEnglishInterfaceText180[]      = "in Slot A or the POWER Button.";
extern const char sEnglishInterfaceText183[]      = "An error has occurred during saving.";
extern const char sEnglishInterfaceText188[]      = "Select a file to load.";
extern const char sEnglishInterfaceText193[]      = "Loading...";
extern const char sEnglishInterfaceText198[]      = "The Chibi-Robo file on the";
extern const char sEnglishInterfaceText199[]      = "Memory Card in Slot A is";
extern const char sEnglishInterfaceText200[]      = "corrupted and must be erased.";
extern const char sEnglishInterfaceText203[]      = "Create new";
extern const char sEnglishInterfaceText204[]      = "game data?";
extern const char sEnglishInterfaceText219[]      = "damaged and cannot be used.";
extern const char sEnglishInterfaceText220[]      = "Insert another Memory Card";
extern const char sEnglishInterfaceText223[]      = "Please insert a different Memory Card";
extern const char sEnglishInterfaceText224[]      = "or press the A Button to continue without saving.";
extern const char sEnglishInterfaceText228[]      = "Save completed.";
extern const char sEnglishInterfaceText233[]      = "Select a file.";
extern const char sEnglishInterfaceText238[]      = "The save data could not be loaded.";
extern const char sEnglishInterfaceText244[]      = "is no longer available.";
extern const char sEnglishInterfaceText250[]      = "Insert a different Memory Card or press the";
extern const char sEnglishInterfaceText251[]      = "A Button to continue without saving.";
extern const char sEnglishInterfaceText256[]      = "press the A Button to continue without saving.";
extern const char sEnglishInterfaceText258[]      = "Chibi-Robo!";
extern const char sEnglishInterfaceText259[]      = "Time Played:";
extern const char sEnglishInterfaceText262[]      = "The Chibi-Robo! demonstration ended. \012Please press the A Button \012to continue "
                                                    "playing.";
extern const char sEnglishInterfaceText263[]      = "This game is compatible";
extern const char sEnglishInterfaceText264[]      = "with 60Hz mode.";
extern const char sEnglishInterfaceText265[]      = "Please select mode.";
extern const char sEnglishInterfaceText266[]      = "If your television does not";
extern const char sEnglishInterfaceText267[]      = "display correctly in 60Hz mode,";
extern const char sEnglishInterfaceText268[]      = "please select 50Hz mode.";
extern const char sEnglishInterfaceText270[]      = "%s%02X 60Hz mode %s  %s%02X 50Hz mode %s";
extern const char sEnglishInterfaceText271[]      = "Screen display has been set to 60Hz mode.";
extern const char sEnglishInterfaceText274[]      = "Screen display has been set to 50Hz mode.";
extern const char sEnglishInterfaceText277[]      = "NEW";

const char* gEnglishInterfaceText[] = {
	sEnglishInterfaceText000,      sEnglishInterfaceText001,      sEnglishInterfaceText002,      sEnglishInterfaceText003,
	sEnglishInterfaceText004,      gEmptyResourceDirectorySuffix, sEnglishInterfaceText003,      sEnglishInterfaceText007,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText012,      sEnglishInterfaceText013,      sEnglishInterfaceText014,      sEnglishInterfaceText015,
	sEnglishInterfaceText016,      sEnglishInterfaceText017,      sEnglishInterfaceText018,      sEnglishInterfaceText019,
	sEnglishInterfaceText020,      sEnglishInterfaceText021,      sEnglishInterfaceText022,      sEnglishInterfaceText023,
	sEnglishInterfaceText024,      sEnglishInterfaceText025,      sEnglishInterfaceText026,      sEnglishInterfaceText027,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText029,      sEnglishInterfaceText030,      sEnglishInterfaceText031,
	sEnglishInterfaceText032,      sEnglishInterfaceText033,      sEnglishInterfaceText034,      sEnglishInterfaceText035,
	sEnglishInterfaceText036,      sEnglishInterfaceText023,      sEnglishInterfaceText038,      sEnglishInterfaceText039,
	sEnglishInterfaceText040,      sEnglishInterfaceText041,      sEnglishInterfaceText042,      sEnglishInterfaceText043,
	sEnglishInterfaceText044,      gEmptyResourceDirectorySuffix, sEnglishInterfaceText046,      sEnglishInterfaceText047,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText057,      sEnglishInterfaceText022,      gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText060,      sEnglishInterfaceText038,      sEnglishInterfaceText062,      sEnglishInterfaceText063,
	sEnglishInterfaceText064,      sEnglishInterfaceText065,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText069,      sEnglishInterfaceText070,      sEnglishInterfaceText071,
	sEnglishInterfaceText072,      sEnglishInterfaceText073,      sEnglishInterfaceText074,      sEnglishInterfaceText075,
	sEnglishInterfaceText076,      sEnglishInterfaceText077,      sEnglishInterfaceText078,      sEnglishInterfaceText079,
	sEnglishInterfaceText080,      sEnglishInterfaceText081,      sEnglishInterfaceText082,      gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText084,      sEnglishInterfaceText085,      sEnglishInterfaceText085,      sEnglishInterfaceText087,
	sEnglishInterfaceText088,      sEnglishInterfaceText088,      sEnglishInterfaceText090,      sEnglishInterfaceText091,
	sEnglishInterfaceText092,      sEnglishInterfaceText093,      sEnglishInterfaceText094,      sEnglishInterfaceText095,
	sEnglishInterfaceText096,      sEnglishInterfaceText097,      sEnglishInterfaceText098,      sEnglishInterfaceText099,
	sEnglishInterfaceText100,      sEnglishInterfaceText064,      sEnglishInterfaceText065,      sEnglishInterfaceText103,
	sEnglishInterfaceText104,      sEnglishInterfaceText105,      sEnglishInterfaceText106,      gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText108,      sEnglishInterfaceText109,      sEnglishInterfaceText110,      sEnglishInterfaceText111,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText113,      sEnglishInterfaceText114,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText118,      sEnglishInterfaceText119,
	sEnglishInterfaceText120,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText123,
	sEnglishInterfaceText124,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText128,      sEnglishInterfaceText129,      sEnglishInterfaceText130,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText113,      sEnglishInterfaceText134,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText113,      sEnglishInterfaceText139,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText143,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText148,      sEnglishInterfaceText149,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText153,      sEnglishInterfaceText154,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText158,      sEnglishInterfaceText159,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText113,
	sEnglishInterfaceText164,      sEnglishInterfaceText165,      sEnglishInterfaceText166,      gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText168,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText173,      sEnglishInterfaceText174,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText178,      sEnglishInterfaceText179,
	sEnglishInterfaceText180,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText183,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText188,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText193,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText198,      sEnglishInterfaceText199,
	sEnglishInterfaceText200,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText203,
	sEnglishInterfaceText204,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText108,      sEnglishInterfaceText109,      sEnglishInterfaceText110,      sEnglishInterfaceText111,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText118,      sEnglishInterfaceText219,
	sEnglishInterfaceText220,      sEnglishInterfaceText105,      sEnglishInterfaceText106,      sEnglishInterfaceText223,
	sEnglishInterfaceText224,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText228,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText233,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText238,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText113,
	sEnglishInterfaceText244,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText118,      sEnglishInterfaceText219,      sEnglishInterfaceText250,      sEnglishInterfaceText251,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText113,      sEnglishInterfaceText114,      sEnglishInterfaceText109,
	sEnglishInterfaceText256,      gEmptyResourceDirectorySuffix, sEnglishInterfaceText258,      sEnglishInterfaceText259,
	sEnglishInterfaceText064,      sEnglishInterfaceText065,      sEnglishInterfaceText262,      sEnglishInterfaceText263,
	sEnglishInterfaceText264,      sEnglishInterfaceText265,      sEnglishInterfaceText266,      sEnglishInterfaceText267,
	sEnglishInterfaceText268,      gEmptyResourceDirectorySuffix, sEnglishInterfaceText270,      sEnglishInterfaceText271,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText274,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText277,
};
#else

extern const char sEnglishInterfaceText000[]      = "Do you want to display";
extern const char sEnglishInterfaceText001[]      = "the game in Progressive Mode?";
extern const char sEnglishInterfaceText002[]      = "%s%02XYes%s  %s%02XNo%s";
extern const char sEnglishInterfaceText003[]      = "Progressive Mode has";
extern const char sEnglishInterfaceText004[]      = "been turned on.";
extern const char gEmptyResourceDirectorySuffix[] = "";
extern const char sEnglishInterfaceText007[]      = "been turned off.";
extern const char sEnglishInterfaceText012[]      = "Store Item";
extern const char sEnglishInterfaceText013[]      = "Remove Suit";
extern const char sEnglishInterfaceText014[]      = "?";
extern const char sEnglishInterfaceText015[]      = "Zoom";
extern const char sEnglishInterfaceText016[]      = "Rumble";
extern const char sEnglishInterfaceText017[]      = "On";
extern const char sEnglishInterfaceText018[]      = "Off";
extern const char sEnglishInterfaceText019[]      = "Sound";
extern const char sEnglishInterfaceText020[]      = "Stereo";
extern const char sEnglishInterfaceText021[]      = "Mono";
extern const char sEnglishInterfaceText022[]      = "Back";
extern const char sEnglishInterfaceText023[]      = "Pause";
extern const char sEnglishInterfaceText024[]      = "Items";
extern const char sEnglishInterfaceText025[]      = "Controller";
extern const char sEnglishInterfaceText026[]      = "Status";
extern const char sEnglishInterfaceText027[]      = "Stickers";
extern const char sEnglishInterfaceText029[]      = "Action";
extern const char sEnglishInterfaceText030[]      = "Cancel";
extern const char sEnglishInterfaceText031[]      = "Rotate Camera";
extern const char sEnglishInterfaceText032[]      = "Equip Screen";
extern const char sEnglishInterfaceText033[]      = "Chibi-Vision";
extern const char sEnglishInterfaceText034[]      = "Pose";
extern const char sEnglishInterfaceText035[]      = "Center Camera";
extern const char sEnglishInterfaceText036[]      = "Map";
extern const char sEnglishInterfaceText038[]      = "Select";
extern const char sEnglishInterfaceText039[]      = "Control Chibi / Select";
extern const char sEnglishInterfaceText040[]      = "Chibi-Ranking";
extern const char sEnglishInterfaceText041[]      = "Chibi-Doors Completed";
extern const char sEnglishInterfaceText042[]      = "Happy Points";
extern const char sEnglishInterfaceText043[]      = "Moolah";
extern const char sEnglishInterfaceText044[]      = "Scrap";
extern const char sEnglishInterfaceText046[]      = "Chibi-PC Menu";
extern const char sEnglishInterfaceText047[]      = "Online Shop";
extern const char sEnglishInterfaceText057[]      = "End";
extern const char sEnglishInterfaceText060[]      = "Buy";
extern const char sEnglishInterfaceText062[]      = "Bought";
extern const char sEnglishInterfaceText063[]      = "Buy?";
extern const char sEnglishInterfaceText064[]      = "Yes";
extern const char sEnglishInterfaceText065[]      = "No";
extern const char sEnglishInterfaceText069[]      = "Recyclotron";
extern const char sEnglishInterfaceText070[]      = "Build";
extern const char sEnglishInterfaceText071[]      = "Exchanged";
extern const char sEnglishInterfaceText072[]      = "Exchange?";
extern const char sEnglishInterfaceText073[]      = "Scrap Trade";
extern const char sEnglishInterfaceText074[]      = "Exchange rate";
extern const char sEnglishInterfaceText075[]      = "Exchange how many?";
extern const char sEnglishInterfaceText076[]      = "You don't have enough Scrap.";
extern const char sEnglishInterfaceText077[]      = "It's ";
extern const char sEnglishInterfaceText078[]      = "OK?";
extern const char sEnglishInterfaceText079[]      = "→";
extern const char sEnglishInterfaceText080[]      = "Build?";
extern const char sEnglishInterfaceText081[]      = "OK, stand by.";
extern const char sEnglishInterfaceText082[]      = "Exchange";
extern const char sEnglishInterfaceText084[]      = "The Disc Cover is open.\012Please close the Disc Cover\012to continue playing.";
extern const char sEnglishInterfaceText085[]      = "Please insert the Chibi-Robo Game Disc.";
extern const char sEnglishInterfaceText087[]      = "The Game Disc could not be read.\012Please read the Nintendo GameCube\012Instruction "
                                                    "Booklet for details.";
extern const char sEnglishInterfaceText088[]      = "Loading";
extern const char sEnglishInterfaceText090[]      = "Score";
extern const char sEnglishInterfaceText091[]      = "Time";
extern const char sEnglishInterfaceText092[]      = "Course";
extern const char sEnglishInterfaceText093[]      = "Enemy";
extern const char sEnglishInterfaceText094[]      = "New Game";
extern const char sEnglishInterfaceText095[]      = "Load Game";
extern const char sEnglishInterfaceText096[]      = "Options";
extern const char sEnglishInterfaceText097[]      = "Settings";
extern const char sEnglishInterfaceText098[]      = "Save";
extern const char sEnglishInterfaceText099[]      = "Empty";
extern const char sEnglishInterfaceText100[]      = "Yes  No";
extern const char sEnglishInterfaceText103[]      = "There is no Memory Card";
extern const char sEnglishInterfaceText104[]      = "in Slot A. Insert a Memory Card";
extern const char sEnglishInterfaceText105[]      = "or press the A Button to";
extern const char sEnglishInterfaceText106[]      = "continue without saving.";
extern const char sEnglishInterfaceText108[]      = "Wrong device in Slot A.";
extern const char sEnglishInterfaceText109[]      = "Please insert a different Memory Card or";
extern const char sEnglishInterfaceText110[]      = "press the A Button to continue";
extern const char sEnglishInterfaceText111[]      = "without saving.";
extern const char sEnglishInterfaceText113[]      = "The Memory Card in Slot A";
extern const char sEnglishInterfaceText114[]      = "cannot be used.";
extern const char sEnglishInterfaceText118[]      = "The Memory Card in Slot A is";
extern const char sEnglishInterfaceText119[]      = "corrupted and needs to be";
extern const char sEnglishInterfaceText120[]      = "formatted. Format now?";
extern const char sEnglishInterfaceText123[]      = "Formatting the Memory Card in Slot A";
extern const char sEnglishInterfaceText124[]      = "will erase all save data. Continue?";
extern const char sEnglishInterfaceText128[]      = "Formatting... Do not touch the";
extern const char sEnglishInterfaceText129[]      = "Memory Card in Slot A";
extern const char sEnglishInterfaceText130[]      = "or the POWER Button.";
extern const char sEnglishInterfaceText134[]      = "could not be formatted.";
extern const char sEnglishInterfaceText139[]      = "has been formatted.";
extern const char sEnglishInterfaceText143[]      = "Continue without saving?";
extern const char sEnglishInterfaceText148[]      = "To create new game data,";
extern const char sEnglishInterfaceText149[]      = "please select a file.";
extern const char sEnglishInterfaceText153[]      = "There is not";
extern const char sEnglishInterfaceText154[]      = "enough free space.";
extern const char sEnglishInterfaceText158[]      = "This data";
extern const char sEnglishInterfaceText159[]      = "is corrupted.";
extern const char sEnglishInterfaceText164[]      = "does not have enough space.";
extern const char sEnglishInterfaceText165[]      = "Chibi-Robo requires 1 file and";
extern const char sEnglishInterfaceText166[]      = "6 blocks to save.";
extern const char sEnglishInterfaceText168[]      = "Overwrite existing file?";
extern const char sEnglishInterfaceText173[]      = "To manage the contents of your Memory";
extern const char sEnglishInterfaceText174[]      = "Card, use the Memory Card screen.";
extern const char sEnglishInterfaceText178[]      = "Now saving...";
extern const char sEnglishInterfaceText179[]      = "Do not touch the Memory Card";
extern const char sEnglishInterfaceText180[]      = "in Slot A or the POWER Button.";
extern const char sEnglishInterfaceText183[]      = "An error has occurred during saving.";
extern const char sEnglishInterfaceText188[]      = "Select a file to load.";
extern const char sEnglishInterfaceText193[]      = "Loading...";
extern const char sEnglishInterfaceText198[]      = "The Chibi-Robo file on the";
extern const char sEnglishInterfaceText199[]      = "Memory Card in Slot A is";
extern const char sEnglishInterfaceText200[]      = "corrupted and must be erased.";
extern const char sEnglishInterfaceText203[]      = "Create new";
extern const char sEnglishInterfaceText204[]      = "game data?";
extern const char sEnglishInterfaceText219[]      = "damaged and cannot be used.";
extern const char sEnglishInterfaceText220[]      = "Insert another Memory Card";
extern const char sEnglishInterfaceText223[]      = "Please insert a different Memory Card";
extern const char sEnglishInterfaceText224[]      = "or press the A Button to continue without saving.";
extern const char sEnglishInterfaceText228[]      = "Save completed.";
extern const char sEnglishInterfaceText233[]      = "Select a file.";
extern const char sEnglishInterfaceText238[]      = "The save data could not be loaded.";
extern const char sEnglishInterfaceText244[]      = "is no longer available.";
extern const char sEnglishInterfaceText250[]      = "Insert a different Memory Card or press the";
extern const char sEnglishInterfaceText251[]      = "A Button to continue without saving.";
extern const char sEnglishInterfaceText256[]      = "press the A Button to continue without saving.";
extern const char sEnglishInterfaceText258[]      = "Chibi-Robo!";
extern const char sEnglishInterfaceText259[]      = "Time Played:";

const char* gEnglishInterfaceText[] = {
	sEnglishInterfaceText000,      sEnglishInterfaceText001,      sEnglishInterfaceText002,      sEnglishInterfaceText003,
	sEnglishInterfaceText004,      gEmptyResourceDirectorySuffix, sEnglishInterfaceText003,      sEnglishInterfaceText007,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText012,      sEnglishInterfaceText013,      sEnglishInterfaceText014,      sEnglishInterfaceText015,
	sEnglishInterfaceText016,      sEnglishInterfaceText017,      sEnglishInterfaceText018,      sEnglishInterfaceText019,
	sEnglishInterfaceText020,      sEnglishInterfaceText021,      sEnglishInterfaceText022,      sEnglishInterfaceText023,
	sEnglishInterfaceText024,      sEnglishInterfaceText025,      sEnglishInterfaceText026,      sEnglishInterfaceText027,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText029,      sEnglishInterfaceText030,      sEnglishInterfaceText031,
	sEnglishInterfaceText032,      sEnglishInterfaceText033,      sEnglishInterfaceText034,      sEnglishInterfaceText035,
	sEnglishInterfaceText036,      sEnglishInterfaceText023,      sEnglishInterfaceText038,      sEnglishInterfaceText039,
	sEnglishInterfaceText040,      sEnglishInterfaceText041,      sEnglishInterfaceText042,      sEnglishInterfaceText043,
	sEnglishInterfaceText044,      gEmptyResourceDirectorySuffix, sEnglishInterfaceText046,      sEnglishInterfaceText047,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText057,      sEnglishInterfaceText022,      gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText060,      sEnglishInterfaceText038,      sEnglishInterfaceText062,      sEnglishInterfaceText063,
	sEnglishInterfaceText064,      sEnglishInterfaceText065,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText069,      sEnglishInterfaceText070,      sEnglishInterfaceText071,
	sEnglishInterfaceText072,      sEnglishInterfaceText073,      sEnglishInterfaceText074,      sEnglishInterfaceText075,
	sEnglishInterfaceText076,      sEnglishInterfaceText077,      sEnglishInterfaceText078,      sEnglishInterfaceText079,
	sEnglishInterfaceText080,      sEnglishInterfaceText081,      sEnglishInterfaceText082,      gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText084,      sEnglishInterfaceText085,      sEnglishInterfaceText085,      sEnglishInterfaceText087,
	sEnglishInterfaceText088,      sEnglishInterfaceText088,      sEnglishInterfaceText090,      sEnglishInterfaceText091,
	sEnglishInterfaceText092,      sEnglishInterfaceText093,      sEnglishInterfaceText094,      sEnglishInterfaceText095,
	sEnglishInterfaceText096,      sEnglishInterfaceText097,      sEnglishInterfaceText098,      sEnglishInterfaceText099,
	sEnglishInterfaceText100,      sEnglishInterfaceText064,      sEnglishInterfaceText065,      sEnglishInterfaceText103,
	sEnglishInterfaceText104,      sEnglishInterfaceText105,      sEnglishInterfaceText106,      gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText108,      sEnglishInterfaceText109,      sEnglishInterfaceText110,      sEnglishInterfaceText111,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText113,      sEnglishInterfaceText114,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText118,      sEnglishInterfaceText119,
	sEnglishInterfaceText120,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText123,
	sEnglishInterfaceText124,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText128,      sEnglishInterfaceText129,      sEnglishInterfaceText130,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText113,      sEnglishInterfaceText134,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText113,      sEnglishInterfaceText139,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText143,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText148,      sEnglishInterfaceText149,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText153,      sEnglishInterfaceText154,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText158,      sEnglishInterfaceText159,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText113,
	sEnglishInterfaceText164,      sEnglishInterfaceText165,      sEnglishInterfaceText166,      gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText168,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText173,      sEnglishInterfaceText174,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText178,      sEnglishInterfaceText179,
	sEnglishInterfaceText180,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText183,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText188,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText193,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText198,      sEnglishInterfaceText199,
	sEnglishInterfaceText200,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText203,
	sEnglishInterfaceText204,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText108,      sEnglishInterfaceText109,      sEnglishInterfaceText110,      sEnglishInterfaceText111,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText118,      sEnglishInterfaceText219,
	sEnglishInterfaceText220,      sEnglishInterfaceText105,      sEnglishInterfaceText106,      sEnglishInterfaceText223,
	sEnglishInterfaceText224,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText228,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText233,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText238,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sEnglishInterfaceText113,
	sEnglishInterfaceText244,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sEnglishInterfaceText118,      sEnglishInterfaceText219,      sEnglishInterfaceText250,      sEnglishInterfaceText251,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText113,      sEnglishInterfaceText114,      sEnglishInterfaceText109,
	sEnglishInterfaceText256,      gEmptyResourceDirectorySuffix, sEnglishInterfaceText258,      sEnglishInterfaceText259,
	sEnglishInterfaceText064,      sEnglishInterfaceText065,
};
#endif
}
