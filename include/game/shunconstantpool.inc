// MWCC addresses the assertion strings relative to the first vector in this pool.
// Keep the data in retail order in both the data owner and the joint-setter translation unit.
#include <dolphin/mtx/mtxtypes.h>

extern "C" {
extern const Vec sShunVectorTemplates[] = { { 1, 1, 1 }, {}, {}, { 1, 1, 1 }, {}, {} };

extern const char sShunPromptRedOn[]                        = "赤つけて";
extern const char sShunPromptRedOff[]                       = "赤消して";
extern const char sShunPromptBlueOn[]                       = "青つけて";
extern const char sShunPromptBlueOff[]                      = "青消して";
extern const char sShunPromptYellowOn[]                     = "黄つけて";
extern const char sShunPromptYellowOff[]                    = "黄消して";
extern const char sShunPromptLeaveRedOffLeaveRedOff[]       = "赤つけないで　赤つけない";
extern const char sShunPromptLeaveRedOffTurnRedOn[]         = "赤つけないで　赤つける";
extern const char sShunPromptLeaveRedOffLeaveBlueOff[]      = "赤つけないで　青つけない";
extern const char sShunPromptLeaveRedOffTurnBlueOn[]        = "赤つけないで　青つける";
extern const char sShunPromptLeaveRedOffLeaveYellowOff[]    = "赤つけないで　黄つけない";
extern const char sShunPromptLeaveRedOffTurnYellowOn[]      = "赤つけないで　黄つける";
extern const char sShunPromptLeaveBlueOffLeaveRedOff[]      = "青つけないで　赤つけない";
extern const char sShunPromptLeaveBlueOffTurnRedOn[]        = "青つけないで　赤つける";
extern const char sShunPromptLeaveBlueOffLeaveBlueOff[]     = "青つけないで　青つけない";
extern const char sShunPromptLeaveBlueOffTurnBlueOn[]       = "青つけないで　青つける";
extern const char sShunPromptLeaveBlueOffLeaveYellowOff[]   = "青つけないで　黄つけない";
extern const char sShunPromptLeaveBlueOffTurnYellowOn[]     = "青つけないで　黄つける";
extern const char sShunPromptLeaveYellowOffLeaveRedOff[]    = "黄つけないで　赤つけない";
extern const char sShunPromptLeaveYellowOffTurnRedOn[]      = "黄つけないで　赤つける";
extern const char sShunPromptLeaveYellowOffLeaveBlueOff[]   = "黄つけないで　青つけない";
extern const char sShunPromptLeaveYellowOffTurnBlueOn[]     = "黄つけないで　青つける";
extern const char sShunPromptLeaveYellowOffLeaveYellowOff[] = "黄つけないで　黄つけない";
extern const char sShunPromptLeaveYellowOffTurnYellowOn[]   = "黄つけないで　黄つける";
extern const char sShunPromptRedOnPlease[]                  = "赤つけてぇ～ん";
extern const char sShunPromptRedOffPlease[]                 = "赤消してよ～";
extern const char sShunPromptTryYellowOn[]                  = "黄つけてみて";
extern const char sShunPromptLetsTurnYellowOff[]            = "黄けしちゃおー";
extern const char sShunPromptTurnRedOn[]                    = "赤つけてー";
extern const char sShunPromptHurryRedOff[]                  = "赤いそいで消してー";
extern const char sShunPromptTurnYellowOff[]                = "黄消しちゃえ！";
extern const char sShunPromptLetsTurnBlueOn[]               = "青つけましょう";
extern const char sShunPromptMaybeBlueOff[]                 = "青消そっかな";
extern const char sShunPromptRedOffThenBlueOff[]            = "赤消したら　すぐ！　青も消す";
extern const char sShunPromptKeepRedAndYellowOn[]           = "赤消さないで　黄も消さない";
extern const char sShunPromptSecondThoughtsRedOff[]         = "赤つけたいけど　やっぱ　つけない";
extern const char sShunPromptKeepYellowAndBlueOff[]         = "黄つけないし　青もつけない";
extern const char sShunPromptYellowOffThenOn[]              = "黄消してみて　黄つけようね";
extern const char sShunPromptKeepYellowOnRedOff[]           = "黄消さないから　赤消そう";
extern const char sShunPromptKeepBlueOffYellowOn[]          = "青つけないけど　黄つけといて";
extern const char sShunPromptBlueOffAnyway[]                = "青けしちゃダメで　青消しちゃう";
extern const char sShunPromptBlueOnThenOff[]                = "青つけてから　青消して";
extern const char sShunPromptRedOnThenLook[]                = "赤つけたら　こっち向いて！";
extern const char sShunPromptTurnYellowOffInstead[]         = "赤　じゃない　青　じゃない　黄消して";
extern const char sShunPromptTurnAllOn[]                    = "えっとー　ぜんぶつけちゃえ！";
extern const char sShunPromptTurnAllOff[]                   = "もう煮えたから　ぜんぶ消して！";
extern const char sShunPromptKeepRedOn[]                    = "やっぱ　赤はついてないとねー　よろしく";
}

extern "C" const u32 sShunZeroWords[60] = {};

struct ShunPositionScale {
	Vec position;
	Vec scale;
};

extern "C" const ShunPositionScale sShunPositionScalePairs[] = {
	{ { -30.0f, 90.71f, -201.0f }, { 1, 1, 1 } },  { { -22.6f, 99.8f, -201.38f }, { 1, 1, 1 } },
	{ { -7.68f, 91.11f, -218.54f }, { 1, 1, 1 } }, { { -0.9f, 99.8f, -219.63f }, { 1, 1, 1 } },
	{ { 13.6f, 90.59f, -201.0f }, { 1, 1, 1 } },   { { 20.94f, 99.8f, -201.38f }, { 1, 1, 1 } },
};

extern "C" const u32 sShunQuizWords[] = { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 4, 2, 0, 0, 0, 0, 4, 2, 0, 0, 0 };

extern "C" {
extern const float sShunFloat0                = 0.0f;
extern const float sShunFloat1                = 1.0f;
extern const float sShunFloat10               = 10.0f;
extern const char sShunJointHeader[]          = "jobj.h";
extern const char sShunJointCondition[]       = "jobj";
extern const char sShunTranslationCondition[] = "translate";
extern const char sShunRotationCondition[]    = "rotate";
extern const char sShunScaleCondition[]       = "scale";
extern const float sShunFloat3                = 3.0f;
extern const float sShunFloat2                = 2.0f;
extern const float sShunFloat300              = 300.0f;
extern const float sShunFloat60               = 60.0f;
extern const float sShunFloat360              = 360.0f;
extern const float sShunFloat120              = 120.0f;
extern const char sShunIntegerFormat[]        = "%d";
extern const float sShunFloat327              = 327.0f;
extern const float sShunFloat80               = 80.0f;
extern const float sShunFloat32               = 32.0f;
extern const float sShunFloat40               = 40.0f;
extern const float sShunFloat600              = 600.0f;
extern const float sShunFloat180              = 180.0f;
extern const float sShunFloat140              = 140.0f;
extern const float sShunFloat28               = 28.0f;
extern const float sShunFloat230              = 230.0f;
extern const float sShunFloat240              = 240.0f;
extern const float sShunFloat430              = 430.0f;
extern const float sShunFloat260              = 260.0f;
extern const float sShunFloat250              = 250.0f;
extern const float sShunFloat36               = 36.0f;
extern const char sShunCorrectAnswerText[]    = "正解○！";
extern const char sShunIncorrectAnswerText[]  = "不正解×！";
extern const char sShunScoreFormat[]          = "%dてん";
}

extern "C" {
extern const double sShunIntToDoubleBias   = 4503601774854144.0;
extern const double sShunDoubleHalf        = 0.5;
extern const double sShunDoubleThree       = 3.0;
extern const double sShunDoubleZero        = 0.0;
extern const float sShunFloatNegativeOne   = -1.0f;
extern const float sShunFloatOnePointTwo   = 1.2f;
extern const float sShunFloatHalf          = 0.5f;
extern const float sShunFloatHalfPi        = 1.5707964f;
extern const float sShunFloatTwoPi         = 6.2831855f;
extern const float sShunFloatPi            = 3.1415927f;
extern const float sShunFloatPoint48       = 0.48f;
extern const float sShunFloat100           = 100.0f;
extern const float sShunFloat5             = 5.0f;
extern const float sShunFloatThreePointSix = 3.6f;
extern const float sShunFloatPointTwo      = 0.2f;

extern const char sShunTypeNameCPad[]           = "CPad";
extern const char sShunTypeNameCVec[]           = "CVec";
extern const char sShunTypeNameVec[]            = "Vec";
extern const char sShunTypeNameCJObj[]          = "CJObj";
extern const char sShunEulerRotationCondition[] = "!(jobj->flags & JOBJ_USE_QUATERNION)";
extern const char sShunTypeNameCStatus[]        = "CStatus";
}
