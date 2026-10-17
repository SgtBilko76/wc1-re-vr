/*
 *  Definitions for the globals declared in include/globals.h.
 *
 *  Provisional: the original scatters these across the modules that own them,
 *  so this file has no counterpart in the shipped image and contributes no
 *  code.  It exists so that regrouping functions cannot break the link.
 */
#include "wc1.h"


/* Original single-precision angular constants, with no identified direct
 * references. Keep their names and ownership unresolved. */
/* 0x004630dc */
const float DAT_004630dc = 3.1415927410125732421875f;
/* 0x004630e0 */
const float DAT_004630e0 = 57.295780181884765625f;
/* 0x004630e4 */
const float DAT_004630e4 = 0.01745329238474369049072265625f;

/* The raster library's tables and variables.  The Win32 build takes them
 * from src/raster.asm, which keeps these first tables in its code segment;
 * the SDL build defines them here for the portable routines. */
#ifdef SDL_PORT
/* Quarter-wave 16.16 cosine table embedded with the original raster
 * assembly.  Indices are tenths of a degree from 0 through 90 degrees. */
const int anRLEQuarterCosine[901] = { /* 0x0043d4bf */
    65536, 65536, 65536, 65535, 65534, 65534, 65532, 65531,
    65530, 65528, 65526, 65524, 65522, 65519, 65516, 65514,
    65510, 65507, 65504, 65500, 65496, 65492, 65488, 65483,
    65479, 65474, 65469, 65463, 65458, 65452, 65446, 65440,
    65434, 65427, 65421, 65414, 65407, 65399, 65392, 65384,
    65376, 65368, 65360, 65352, 65343, 65334, 65325, 65316,
    65306, 65296, 65287, 65277, 65266, 65256, 65245, 65234,
    65223, 65212, 65201, 65189, 65177, 65165, 65153, 65140,
    65128, 65115, 65102, 65088, 65075, 65061, 65048, 65033,
    65019, 65005, 64990, 64975, 64960, 64945, 64930, 64914,
    64898, 64882, 64866, 64850, 64833, 64816, 64799, 64782,
    64765, 64747, 64729, 64711, 64693, 64675, 64656, 64637,
    64618, 64599, 64580, 64560, 64540, 64520, 64500, 64480,
    64459, 64439, 64418, 64397, 64375, 64354, 64332, 64310,
    64288, 64266, 64243, 64220, 64197, 64174, 64151, 64128,
    64104, 64080, 64056, 64032, 64007, 63983, 63958, 63933,
    63907, 63882, 63856, 63830, 63804, 63778, 63752, 63725,
    63698, 63671, 63644, 63617, 63589, 63562, 63534, 63505,
    63477, 63449, 63420, 63391, 63362, 63332, 63303, 63273,
    63243, 63213, 63183, 63152, 63122, 63091, 63060, 63029,
    62997, 62966, 62934, 62902, 62870, 62837, 62805, 62772,
    62739, 62706, 62672, 62639, 62605, 62571, 62537, 62503,
    62468, 62434, 62399, 62364, 62328, 62293, 62257, 62222,
    62186, 62149, 62113, 62076, 62040, 62003, 61966, 61928,
    61891, 61853, 61815, 61777, 61739, 61700, 61662, 61623,
    61584, 61544, 61505, 61465, 61426, 61386, 61346, 61305,
    61265, 61224, 61183, 61142, 61101, 61059, 61018, 60976,
    60934, 60892, 60849, 60807, 60764, 60721, 60678, 60635,
    60591, 60547, 60504, 60459, 60415, 60371, 60326, 60281,
    60236, 60191, 60146, 60100, 60055, 60009, 59963, 59917,
    59870, 59824, 59777, 59730, 59683, 59635, 59588, 59540,
    59492, 59444, 59396, 59347, 59299, 59250, 59201, 59152,
    59102, 59053, 59003, 58953, 58903, 58853, 58803, 58752,
    58701, 58650, 58599, 58548, 58497, 58445, 58393, 58341,
    58289, 58236, 58184, 58131, 58078, 58025, 57972, 57918,
    57865, 57811, 57757, 57703, 57649, 57594, 57539, 57485,
    57430, 57374, 57319, 57264, 57208, 57152, 57096, 57040,
    56983, 56927, 56870, 56813, 56756, 56699, 56641, 56583,
    56526, 56468, 56410, 56351, 56293, 56234, 56175, 56116,
    56057, 55998, 55938, 55879, 55819, 55759, 55699, 55638,
    55578, 55517, 55456, 55395, 55334, 55273, 55211, 55149,
    55087, 55025, 54963, 54901, 54838, 54775, 54713, 54650,
    54586, 54523, 54459, 54396, 54332, 54268, 54204, 54139,
    54075, 54010, 53945, 53880, 53815, 53749, 53684, 53618,
    53552, 53486, 53420, 53354, 53287, 53221, 53154, 53087,
    53020, 52952, 52885, 52817, 52750, 52682, 52613, 52545,
    52477, 52408, 52339, 52270, 52201, 52132, 52063, 51993,
    51923, 51854, 51784, 51713, 51643, 51573, 51502, 51431,
    51360, 51289, 51218, 51146, 51075, 51003, 50931, 50859,
    50787, 50714, 50642, 50569, 50496, 50423, 50350, 50277,
    50203, 50130, 50056, 49982, 49908, 49834, 49760, 49685,
    49610, 49536, 49461, 49386, 49310, 49235, 49159, 49084,
    49008, 48932, 48856, 48779, 48703, 48626, 48549, 48472,
    48395, 48318, 48241, 48163, 48086, 48008, 47930, 47852,
    47774, 47695, 47617, 47538, 47459, 47380, 47301, 47222,
    47143, 47063, 46983, 46904, 46824, 46744, 46663, 46583,
    46502, 46422, 46341, 46260, 46179, 46098, 46016, 45935,
    45853, 45771, 45689, 45607, 45525, 45443, 45360, 45278,
    45195, 45112, 45029, 44946, 44862, 44779, 44695, 44612,
    44528, 44444, 44360, 44275, 44191, 44107, 44022, 43937,
    43852, 43767, 43682, 43597, 43511, 43425, 43340, 43254,
    43168, 43082, 42995, 42909, 42823, 42736, 42649, 42562,
    42475, 42388, 42301, 42213, 42126, 42038, 41950, 41862,
    41774, 41686, 41598, 41509, 41421, 41332, 41243, 41154,
    41065, 40976, 40887, 40797, 40708, 40618, 40528, 40438,
    40348, 40258, 40167, 40077, 39986, 39896, 39805, 39714,
    39623, 39532, 39441, 39349, 39258, 39166, 39074, 38982,
    38890, 38798, 38706, 38614, 38521, 38428, 38336, 38243,
    38150, 38057, 37964, 37870, 37777, 37684, 37590, 37496,
    37402, 37308, 37214, 37120, 37026, 36931, 36837, 36742,
    36647, 36552, 36457, 36362, 36267, 36172, 36076, 35981,
    35885, 35789, 35693, 35597, 35501, 35405, 35309, 35212,
    35116, 35019, 34923, 34826, 34729, 34632, 34535, 34437,
    34340, 34242, 34145, 34047, 33949, 33852, 33754, 33655,
    33557, 33459, 33361, 33262, 33163, 33065, 32966, 32867,
    32768, 32669, 32570, 32470, 32371, 32271, 32172, 32072,
    31972, 31872, 31772, 31672, 31572, 31472, 31372, 31271,
    31171, 31070, 30969, 30868, 30767, 30666, 30565, 30464,
    30363, 30261, 30160, 30058, 29956, 29855, 29753, 29651,
    29549, 29447, 29344, 29242, 29140, 29037, 28935, 28832,
    28729, 28626, 28523, 28420, 28317, 28214, 28111, 28007,
    27904, 27800, 27697, 27593, 27489, 27385, 27281, 27177,
    27073, 26969, 26865, 26760, 26656, 26551, 26447, 26342,
    26237, 26132, 26027, 25922, 25817, 25712, 25607, 25502,
    25396, 25291, 25185, 25080, 24974, 24868, 24762, 24656,
    24550, 24444, 24338, 24232, 24125, 24019, 23913, 23806,
    23699, 23593, 23486, 23379, 23272, 23165, 23058, 22951,
    22844, 22737, 22629, 22522, 22415, 22307, 22200, 22092,
    21984, 21876, 21769, 21661, 21553, 21445, 21336, 21228,
    21120, 21012, 20903, 20795, 20686, 20578, 20469, 20360,
    20252, 20143, 20034, 19925, 19816, 19707, 19598, 19489,
    19380, 19270, 19161, 19051, 18942, 18832, 18723, 18613,
    18504, 18394, 18284, 18174, 18064, 17954, 17844, 17734,
    17624, 17514, 17403, 17293, 17183, 17072, 16962, 16851,
    16741, 16630, 16520, 16409, 16298, 16187, 16076, 15966,
    15855, 15744, 15633, 15521, 15410, 15299, 15188, 15077,
    14965, 14854, 14742, 14631, 14519, 14408, 14296, 14185,
    14073, 13961, 13849, 13738, 13626, 13514, 13402, 13290,
    13178, 13066, 12954, 12842, 12729, 12617, 12505, 12393,
    12280, 12168, 12055, 11943, 11831, 11718, 11605, 11493,
    11380, 11268, 11155, 11042, 10929, 10817, 10704, 10591,
    10478, 10365, 10252, 10139, 10026, 9913, 9800, 9687,
    9574, 9461, 9347, 9234, 9121, 9008, 8894, 8781,
    8668, 8554, 8441, 8327, 8214, 8100, 7987, 7873,
    7760, 7646, 7533, 7419, 7305, 7192, 7078, 6964,
    6850, 6737, 6623, 6509, 6395, 6281, 6167, 6054,
    5940, 5826, 5712, 5598, 5484, 5370, 5256, 5142,
    5028, 4914, 4800, 4686, 4572, 4457, 4343, 4229,
    4115, 4001, 3887, 3773, 3658, 3544, 3430, 3316,
    3201, 3087, 2973, 2859, 2744, 2630, 2516, 2401,
    2287, 2173, 2059, 1944, 1830, 1716, 1601, 1487,
    1372, 1258, 1144, 1029, 915, 801, 686, 572,
    458, 343, 229, 114, 0
};

/* IFF chunk identifiers embedded between the original raster routines. */
const unsigned char abIffBmhdChunkId[4] = { 'B', 'M', 'H', 'D' }; /* 0x0043e778 */
const unsigned char abIffCmapChunkId[4] = { 'C', 'M', 'A', 'P' }; /* 0x0043e77c */
const unsigned char abIffBodyChunkId[4] = { 'B', 'O', 'D', 'Y' }; /* 0x0043e780 */
#endif

/* 0x0046da90 */
InputEvent *pInputEventHead = 0;

/* 0x0046da94 */
InputEvent *pInputEventTail = 0;

/* 0x0046da98 */
int bInputEventPoolInitialized = 0;

/* 0x0046da9c */
unsigned char *pDrawnMouseCursorShape = 0;

/* 0x0046daa0 */
int nMouseCursorShowCount = 0;

/* 0x0046daa4 */
short nScreenWidth = 320;

/* 0x0046daa8 */
short nScreenHeight = 200;

/* 0x0046dab8 */
ScreenViewportGeometry aScreenViewportGeometry[6] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 320, 128, 0, 24, 0, 24, -24576, -1 },
    { 320, 200, 0, 0, 0, 0, -1536, -1 }
};

/* 0x0046db28 */
signed char acDirectionShapeFrame[DIRECTION_VIEW_COUNT * DIRECTION_SHAPE_TABLE_COUNT] = {
    0, 1, 2, 3, 4, 5, 6, 7, 6, 5, 4, 3, 2,
    14, 13, 12, 11, 10, 9, 8, 9, 10, 11, 12, 13,
    15, 16, 17, 18, 19, 20, 21, 20, 19, 18, 17, 16,
    28, 27, 26, 25, 24, 23, 22, 23, 24, 25, 26, 27,
    29, 30, 31, 32, 33, 34, 35, 34, 33, 32, 31, 30,
    36,
    0, 1, 2, 3, 4, 5, 6, 7, 6, 5, 4, 3, 2,
    14, 13, 12, 11, 10, 9, 8, 9, 10, 11, 12, 13,
    15, 16, 17, 18, 19, 20, 21, 20, 19, 18, 17, 16,
    14, 13, 12, 11, 10, 9, 8, 9, 10, 11, 12, 13,
    1, 2, 3, 4, 5, 6, 7, 6, 5, 4, 3, 2, 0,
    0, 1, 2, 3, 1, 3, 2, 1, 2, 3, 1, 3, 2,
    4, 5, 6, 4, 6, 5, 4, 5, 6, 4, 6, 5,
    7, 8, 9, 7, 9, 8, 7, 8, 9, 7, 9, 8,
    10, 11, 12, 10, 12, 11, 10, 11, 12, 10, 12, 11,
    13, 14, 15, 13, 15, 14, 13, 14, 15, 13, 15, 14, 16
};

/* 0x0046dbe8 */
signed char acDirectionShapeFlip[DIRECTION_VIEW_COUNT * DIRECTION_SHAPE_TABLE_COUNT] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 2,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0
};

/* 0x0046dca8 */
short aasCockpitLightX[5][7] = {
    {101, 101, 189, 137, 265, -99, 203},
    {198, 198, 133, 145, 234, 234, -99},
    {76, 76, 139, 257, 20, 20, -99},
    {194, 194, 96, 137, 96, 96, -99},
    {-99, -99, 179, 133, -99, -99, -99}
};

/* 0x0046dcf0 */
short aasCockpitLightY[5][7] = {
    {122, 145, 121, 7, 121, -99, 147},
    {120, 139, 114, 114, 179, 188, -99},
    {164, 183, 22, 181, 162, 178, -99},
    {119, 142, 136, 15, 126, 116, -99},
    {-99, -99, 115, 114, -99, -99, -99}
};

/* 0x0046dd38 */
signed char aacCockpitLightOffFrame[5][7] = {
    {14, 14, 14, 11, 13, 14, 14},
    {12, 12, 12, 13, 12, 12, 12},
    {9, 9, 9, 14, 9, 9, 9},
    {15, 15, 13, 11, 13, 13, 13},
    {5, 5, 5, 5, 5, 5, 5}
};

/* 0x0046dd60 */
signed char aacCockpitLightOnFrame[5][7] = {
    {3, 3, 3, 0, 2, 3, 3},
    {2, 2, 2, 3, 2, 2, 2},
    {1, 1, 1, 6, 1, 1, 1},
    {4, 4, 2, 0, 2, 2, 2},
    {1, 1, 1, 1, 1, 1, 1}
};

/* 0x0046dd88 */
CockpitBarDefinition aaCockpitBars[5][8] = {
    {
        {0, 215, 122, 219, 152, 31, 16, 5},
        {2, 143, 97, 175, 99, 33, 12, 1},
        {0, 114, 127, 123, 130, 4, 17, 6},
        {1, 114, 143, 123, 146, 4, 20, 9},
        {2, 109, 132, 112, 141, 4, 18, 7},
        {3, 125, 132, 128, 141, 4, 19, 8},
        {0, 109, 122, 128, 125, 4, 15, 4},
        {1, 109, 148, 128, 151, 4, 21, 10}
    },
    {
        {2, 96, 14, 134, 17, 39, 10, 0},
        {2, 185, 14, 223, 17, 39, 11, 1},
        {0, 213, 124, 220, 127, 4, 15, 5},
        {1, 213, 138, 220, 141, 4, 18, 8},
        {2, 205, 129, 208, 136, 4, 16, 6},
        {3, 225, 129, 228, 136, 4, 17, 7},
        {0, 205, 116, 228, 122, 7, 14, 4},
        {1, 205, 143, 228, 149, 7, 19, 9}
    },
    {
        {2, 249, 165, 293, 169, 45, 11, 3},
        {2, 131, 13, 189, 17, 59, 8, 0},
        {0, -99, -99, -99, -99, 0, 0, 0},
        {1, -99, -99, -99, -99, 0, 0, 0},
        {2, 45, 172, 51, 181, 7, 12, 4},
        {3, 70, 172, 76, 181, 7, 13, 5},
        {0, 49, 164, 72, 170, 7, 10, 2},
        {1, 49, 183, 72, 189, 7, 15, 7}
    },
    {
        {0, 123, 118, 126, 148, 31, 14, 3},
        {2, 144, 100, 174, 102, 31, 12, 1},
        {0, 207, 124, 216, 127, 4, 17, 6},
        {1, 207, 140, 216, 143, 4, 20, 9},
        {2, 202, 129, 205, 138, 4, 18, 7},
        {3, 218, 129, 221, 138, 4, 19, 8},
        {0, 202, 119, 221, 122, 4, 16, 5},
        {1, 202, 145, 221, 148, 4, 21, 10}
    },
    {
        {0, 180, 124, 184, 146, 22, 6, 2},
        {2, 143, 110, 175, 112, 33, 4, 0},
        {0, -99, -99, 0, 0, 0, 0, 0},
        {0, -99, -99, 0, 0, 0, 0, 0},
        {0, -99, -99, 0, 0, 0, 0, 0},
        {0, -99, -99, 0, 0, 0, 0, 0},
        {0, 134, 124, 138, 146, 32, 7, 3},
        {0, -99, -99, 0, 0, 0, 0, 0}
    }
};

/* 0x0046e008 */
CockpitLayout stCockpitLayout = {
    {
        {{99, 130}, {227, 122}, {38, 164}, {192, 126},
         {-99, -99}, {0, 0}},
        {{99, 140}, {227, 140}, {38, 184}, {192, 136},
         {-99, -99}, {0, 0}},
        {{201, 105}, {156, 14}, {197, 145}, {262, 102},
         {219, 115}, {0, 0}},
        {{112, 105}, {156, 19}, {105, 145}, {38, 102},
         {71, 115}, {0, 0}}
    },
    {
        {10, 133, 82, 198}, {0, 99, 73, 165}, {6, 1, 78, 66},
        {11, 111, 84, 176}, {48, 126, 120, 191}
    },
    {
        {236, 133, 309, 198}, {246, 97, 319, 165},
        {241, 1, 313, 66}, {235, 111, 308, 176},
        {198, 126, 270, 191}
    },
    {
        {159, 126, 145, 113, 173, 139},
        {103, 132, 89, 119, 117, 145},
        {160, 39, 146, 26, 174, 52},
        {159, 129, 145, 116, 173, 142},
        {159, 130, 145, 117, 173, 143}
    },
    {0, 0},
    {
        {120, 152, 203, 199}, {120, 152, 205, 199},
        {120, 151, 213, 199}, {120, 152, 205, 199},
        {120, 152, 203, 199}
    }
};

/* 0x0046e120 */
short asPilotHandOrigins[10] = {
    154, 187, 154, 187, 154, 187, 154, 187, 154, 187
};

/* The original contains ten initialized signed words here. Their
 * owner and purpose are not established by any identified reference. */
/* 0x0046e138 */
short DAT_0046e138[10] = {
    -400, -400, -400, -400, -400,
    -400, -400, -400, -400, -400
};

#ifdef SDL_PORT
void *apRasterDriverCallbacks[13]; /* 0x0046e69c */
char szRasterDriverName[13] = "MCGA.DLL"; /* 0x0046e6d0 */
unsigned char *pRLEEncodeBuffer; /* 0x0046e6dd */
int nRLEPendingSkip; /* 0x0046e6e1 */
unsigned char *pRLEScanlineStart; /* 0x0046e6e5 */
unsigned char *pRLEScanCursor; /* 0x0046e6e9 */
unsigned char *pRLEOutputCursor; /* 0x0046e6ed */
unsigned char *pRLELiteralStart; /* 0x0046e6f1 */
int nRLEEncodedMinimumX; /* 0x0046e705 */
int nRLEEncodedMinimumY; /* 0x0046e709 */
int nRLEEncodedMaximumX; /* 0x0046e70d */
int nRLEEncodedMaximumY; /* 0x0046e711 */
unsigned char abRasterDecodeBuffer[0xd00]; /* 0x0046ec15 */
unsigned char abRasterPaletteUsageScratch[0x300]; /* 0x0046f915 */
unsigned char abRasterFadeAccumulator[0x300]; /* 0x0046fc15 */
unsigned char abGifLZWCodeMasks[9] = { /* 0x0046ff15 */
    0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff
};
unsigned char abGifInterlaceRowSteps[5] = { /* 0x0046ff1e */
    8, 8, 4, 2, 0
};
unsigned char abGifInterlaceRowStarts[5] = { /* 0x0046ff23 */
    0, 4, 2, 1, 0
};
RasterClip *pGifRasterTarget; /* 0x0046ff28 */
unsigned char abRasterPaletteTranslation[256]; /* 0x0046ff2c */
RLETransformVertex aRLETransformVertices[4]; /* 0x0047002c */
int anRLESourceSteps[4]; /* 0x0047007c */
#endif
int nInsertKeyState; /* 0x00475b68 */
int nClearedKeyboardState; /* 0x00475b6c */
int nOemPeriodKeyState; /* 0x00475bf0 */
int nHomeKeyState; /* 0x00475bf4 */
int nPageDownKeyState; /* 0x00475bf8 */
int nOemCommaKeyState; /* 0x00475bfc */
int nRightArrowKeyState; /* 0x00475c00 */
int nEndKeyState; /* 0x00475c04 */
int nClearKeyState; /* 0x00475c08 */
int nDeleteKeyState; /* 0x00475c0c */
int nUpArrowKeyState; /* 0x00475c10 */
int nDownArrowKeyState; /* 0x00475c14 */
char szStreamsPath[0x100]; /* 0x00475c18 */
int nPageUpKeyState; /* 0x00475d18 */
int nLeftArrowKeyState; /* 0x00475d1c */
char szReadDataFileError[0x40]; /* 0x00475d20 */
char szCreateDataFileError[0x40]; /* 0x00475d60 */
char szWriteDataFileError[0x40]; /* 0x00475da0 */
char szSeekDataFileError[0x80]; /* 0x00475de0 */
int bMonoDebugInstalled; /* 0x00475e70 */
HANDLE hMonoDebugDevice; /* 0x00475e74 */
int nFlightFrameTotalTicks; /* 0x004764f8 */
int nFlightCockpitTicks; /* 0x004764fc */
LARGE_INTEGER liFlightAfterInput; /* 0x00476500 */
LARGE_INTEGER liFlightFrameEnd; /* 0x00476508 */
int nFlightPresentTicks; /* 0x00476510 */
LARGE_INTEGER liFlightFrameStart; /* 0x00476518 */
LARGE_INTEGER liFlightAfterSimulation; /* 0x00476520 */
int nFlightSimulationTicks; /* 0x00476528 */
LARGE_INTEGER liFlightAfterCockpit; /* 0x00476530 */
int nFlightInputTicks; /* 0x00476538 */
LARGE_INTEGER liFlightAfterRender; /* 0x00476540 */
int nFlightRenderTicks; /* 0x00476548 */
unsigned char szFrameRateText[32]; /* 0x00476620 */
unsigned int DAT_00476640;
unsigned int DAT_00476644;
unsigned char *pDIBPixelBuffer; /* 0x00476648 */
unsigned int nDIBWidth; /* 0x0047664c */
unsigned int nDIBHeight; /* 0x00476650 */
unsigned char abDIBPixelBackup[0xfa00]; /* 0x00476658 */
int DAT_00486058;
int nDIBSlamCount; /* 0x00486060 */
HWND hDIBWindow; /* 0x00486074 */
char szDIBErrorMessage[0x98]; /* 0x00486078 */
unsigned char abDIBPaletteCache[1024]; /* 0x00486110 */
float fMeasuredFrameRate; /* 0x00486510 */
int bDIBSlamPending; /* 0x00486518 */
char szSpaceFlightMaxFpsMessage[0x80]; /* 0x00486528 */
RasterClip stRasterClip; /* 0x00496fc0 */
unsigned char abSolidColourTranslation[256]; /* 0x00497648 */
unsigned char abShapeRLEScratch[0x100000]; /* 0x00497748 */
extern unsigned int dwStreamerState; /* 0x00597cd0 */
int bStreamerAudioPlaying; /* 0x00597748 */
char szStreamerPath[128]; /* 0x00597750 */
int DAT_00598888;
int DAT_0059888c;
int DAT_00598890;
Viewport stRoomScreenViewport; /* 0x005988a0 */
short nSavedRoomControllerX; /* 0x005988b4 */
unsigned char *pRecRoomRoster; /* 0x005988b8 */
unsigned char *pMissionCampaignData; /* 0x005988bc */
unsigned char *apRecRoomCharacterShapes[3]; /* 0x005988c0 */
PilotRecord aPilotRecords[9]; /* 0x005988d0 */
ConstellationObjectDefinition *pConstellationDefinitions; /* 0x00598a28 */
PilotRecord *apWingmanPilots[8]; /* 0x00598a30 */
unsigned char *pRecRoomBackgroundShape; /* 0x00598a50 */
Viewport stRoomDisplayViewport; /* 0x00598a60 */
Viewport stRoomMouseViewport; /* 0x00598a80 */
unsigned char *apRecRoomTextData[3]; /* 0x00598aa0 */
short nRoomMenuCursorFrame; /* 0x00598ab0 */
const TitleMenuRegion *pRoomMenuRegions; /* 0x00598ab2 */
char **ppszRoomMenuLabels; /* 0x00598ab6 */
const char *pszCurrentRoomMenuLabel; /* 0x00598aba */
TextContext stRoomMenuTextContext; /* 0x00598abe */
unsigned char *apRecRoomSceneData[3]; /* 0x00598ae0 */
unsigned char *pBriefingPacket; /* 0x00598aec */
unsigned char *pBriefingTextData; /* 0x00598af0 */
short *pMouthAnimationCommands; /* 0x00598af4 */
unsigned char *pMedalTextData; /* 0x00598af8 */
unsigned char *pDebriefingSceneData; /* 0x00598afc */
char szTextScratchBuffer[256]; /* 0x00598b00 */
unsigned char *pBriefingSceneData; /* 0x00598c00 */
unsigned char *pConversationBackdropShape; /* 0x00598c04 */
short nConversationMedalIndex; /* 0x00598c08 */
short nConversationSceneType; /* 0x00598c0a */
unsigned char *pTalkingHeadShape; /* 0x00598c0c */
short nConversationTextColour; /* 0x00598c10 */
unsigned char *pBriefingAnimationShape; /* 0x00598c14 */
short *pFaceAnimationCommands; /* 0x00598c18 */
unsigned char *pBriefingBodyShape; /* 0x00598c1c */
unsigned char *pMedalSceneData; /* 0x00598c20 */
unsigned char *pBriefingPortraitShape; /* 0x00598c24 */
unsigned char *pDebriefingTextData; /* 0x00598c28 */
unsigned char *pBriefingCloseupShape; /* 0x00598c2c */
unsigned char *pConversationOverlayShape; /* 0x00598c30 */
unsigned char bFilteredKeyWaitStarted; /* 0x00598c34 */
InputEvent aInputEventPool[0x100]; /* 0x00598c40 */
int nMouseCursorSavedY; /* 0x0059a840 */
int nMouseCursorSavedX; /* 0x0059a844 */
unsigned char bInputMode; /* 0x0059a848 */
unsigned char bFilteredKeyWaitActive; /* 0x0059a849 */
int bMouseCursorDrawn; /* 0x0059a84c */
short nEventManagerActive; /* 0x0059a850 */
short nViewCenterX; /* 0x0059a852 */
short nViewCenterY; /* 0x0059a854 */
short bExpandedMemoryAvailable; /* 0x0059a856 */
short nPacketDecompressSourceFile; /* 0x0059a858 */
unsigned char abInputKeyState[0x80]; /* 0x0059a860 */
int nMouseCursorDrawnY; /* 0x0059a8e0 */
int nMouseCursorDrawnX; /* 0x0059a8e4 */
int anObjectDepthPlaced[SPACE_OBJECT_COUNT]; /* 0x0059a8f0 */
short DAT_0059a9f0;
signed char cScreenViewportMode; /* 0x0059a9f2 */
const ScreenViewportGeometry * volatile
    pScreenViewportGeometry; /* 0x0059a9f4 */
int anSortedObject[SPACE_OBJECT_COUNT]; /* 0x0059aa00 */
short nPacketDecompressInputPosition; /* 0x0059ab00 */
void *pPacketDecompressInput; /* 0x0059ab04 */
unsigned short wCurrentInputModifiers; /* 0x0059ab08 */
MouseCursorState stMouseCursorState; /* 0x0059ab10 */
void (*pEventManagerPump)(void); /* 0x0059ab2c */
int nPacketDecompressResult; /* 0x0059ab30 */
short DAT_0059ab34;
short nPacketDecompressPending; /* 0x0059ab36 */
unsigned short wPacketDecompressInputSize; /* 0x0059ab38 */
short nPacketDecompressWorkspaceSegment; /* 0x0059ab3a */
volatile int bFrameTickPending; /* 0x0059ab3c */
int bMouseCursorDamagePending; /* 0x0059ab40 */
int nMouseCursorDamageRight; /* 0x0059ab44 */
int nMouseCursorDamageBottom; /* 0x0059ab48 */
int DAT_0059ab4c;
short nViewportOriginY; /* 0x0059ab50 */
short nViewportOriginX; /* 0x0059ab52 */
unsigned int nTickCount60Hz; /* 0x0059ab54 */
unsigned char bEscapePressed; /* 0x0059ab58 */
int nMouseCursorDamageLeft; /* 0x0059ab5c */
int nMouseCursorDamageTop; /* 0x0059ab60 */
short DAT_0059ab64;
int aiInputEventSlotUsed[0x100]; /* 0x0059ab70 */
MouseCursorState stHostMouseState; /* 0x0059af70 */
TextContext *pCurrentTextContext; /* 0x0059af8c */
int nInputTickScale; /* 0x0059af90 */
FixedVector aObjectViewPosition[SPACE_OBJECT_COUNT]; /* 0x0059afa0 */
short anObjectPitchRotation[SPACE_OBJECT_COUNT]; /* 0x0059b2a0 */
int anShipSpeed[64]; /* 0x0059b320 */
short nSpaceFrame; /* 0x0059b420 */
unsigned int anShipAlertFlags[12]; /* 0x0059b430 */
FixedVector vPlayerAcceleration; /* 0x0059b460 */
int anShipFuel[12]; /* 0x0059b470 */
short asObjectDistance[SPACE_OBJECT_COUNT]; /* 0x0059b4a0 */
ShortVector aShipFormationOffset[10]; /* 0x0059b520 */
enum ObjectType aeObjectType[SPACE_OBJECT_COUNT]; /* 0x0059b560 */
short asObjectAnimationDelay[SPACE_OBJECT_COUNT]; /* 0x0059b660 */
FixedVector aShipRightVector[64]; /* 0x0059b6e0 */
FixedVector aShipUpVector[64]; /* 0x0059b9e0 */
FixedVector aShipForwardVector[64]; /* 0x0059bce0 */
HazardField *pActiveHazardField; /* 0x0059bfe0 */
signed char acPlayerComponentDamage[9]; /* 0x0059bff0 */
signed char abFlightPath[MISSION_OBJECTIVE_COUNT]; /* 0x0059c000 */
FixedVector aShipVelocity[SPACE_OBJECT_COUNT]; /* 0x0059c010 */
short anYawGoal[16]; /* 0x0059c310 */
short asObjectCounter[SPACE_OBJECT_COUNT]; /* 0x0059c330 */
short nTargetShip; /* 0x0059c3b0 */
enum SpecialManeuver aeSpecialManeuver[12]; /* 0x0059c3c0 */
enum ShipMissionType aeShipMissionType[12]; /* 0x0059c3f0 */
short asShipCount[16]; /* 0x0059c420 */
short asShipMaximumSpeed[16]; /* 0x0059c440 */
signed char acShipDamage[10]; /* 0x0059c460 */
signed char cMissionObjectiveCount; /* 0x0059c46a */
short asViableTargetDistance[16]; /* 0x0059c470 */
FixedVector aShipPosition[SPACE_OBJECT_COUNT]; /* 0x0059c490 */
unsigned short awObjectMass[SPACE_OBJECT_COUNT]; /* 0x0059c790 */
short asShipAfterburnerTimer[16]; /* 0x0059c810 */
short nShipMissionIndices[10]; /* 0x0059c830 */
signed char acShipCommunicator[16]; /* 0x0059c850 */
FixedVector vStarFieldMotion; /* 0x0059c860 */
signed char cCurrentNavPointIndex; /* 0x0059c86c */
short asObjectFlip[SPACE_OBJECT_COUNT]; /* 0x0059c870 */
short nEyeRollGoal; /* 0x0059c8f0 */
FixedVector vPreviousStarFieldMotion; /* 0x0059c900 */
short nHazardFieldCount; /* 0x0059c90c */
signed char acLastAttacker[16]; /* 0x0059c910 */
signed char acViableTarget[16]; /* 0x0059c920 */
short asActionCount[10]; /* 0x0059c930 */
short nEyeYawGoal; /* 0x0059c944 */
short asObjectScreenScale[SPACE_OBJECT_COUNT]; /* 0x0059c950 */
short asObjectAfterburnerVelocity[SPACE_OBJECT_COUNT]; /* 0x0059c9d0 */
CampaignState stCampaignState; /* 0x0059ca50 */
unsigned char aShipWeapons[10][0x47]; /* 0x0059cab0 */
signed char acShipRating[16]; /* 0x0059cd80 */
short asObjectScreenAngle[SPACE_OBJECT_COUNT]; /* 0x0059cd90 */
short nTargetRange; /* 0x0059ce10 */
unsigned int anVduModeCache[2]; /* 0x0059ce18 */
signed char acObjectOwner[64]; /* 0x0059ce20 */
signed char acShipTarget[16]; /* 0x0059ce60 */
short nPreviousPitchInput; /* 0x0059ce70 */
short nPreviousYawInput; /* 0x0059ce72 */
short nPreviousRollInput; /* 0x0059ce74 */
short anObjectYawRotation[SPACE_OBJECT_COUNT]; /* 0x0059ce80 */
signed char acPilotHitPoints[16]; /* 0x0059cf00 */
signed char acTurnRegulator[16]; /* 0x0059cf10 */
unsigned char abCollisionAlertTarget[0x10]; /* 0x0059cf20 */
int aiPilotLevel[12]; /* 0x0059cf30 */
short asTargetListRange[16]; /* 0x0059cf60 */
short asObjectDrawY[SPACE_OBJECT_COUNT]; /* 0x0059cf80 */
short asObjectDrawX[SPACE_OBJECT_COUNT]; /* 0x0059d000 */
short asPreviousObjectDistance[SPACE_OBJECT_COUNT]; /* 0x0059d080 */
enum ObjectClass aeObjectClass[SPACE_OBJECT_COUNT]; /* 0x0059d100 */
enum ShipObjective aeShipObjective[12]; /* 0x0059d200 */
short asObjectViewFrame[SPACE_OBJECT_COUNT]; /* 0x0059d230 */
int nCurrentObjectiveRange; /* 0x0059d2b0 */
signed char acWingmanMessageState[16]; /* 0x0059d2c0 */
short asCollisionCountdown[16]; /* 0x0059d2d0 */
unsigned char *apObjectShape[SPACE_OBJECT_COUNT]; /* 0x0059d2f0 */
short nPitchInput; /* 0x0059d3f0 */
short nYawInput; /* 0x0059d3f2 */
short nRollInput; /* 0x0059d3f4 */
short asShipWingLeader[16]; /* 0x0059d400 */
/* The Win32 object allocator reserves slots 0 through 9 for live ships. */
short aasShipArmor[10][4]; /* 0x0059d420 */
short asShipWeaponEnergy[10]; /* 0x0059d470 */
signed char acFormationMemberList[16]; /* 0x0059d490 */
signed char acShipIonDriveDamage[16]; /* 0x0059d4a0 */
short anShipMissionShip[16]; /* 0x0059d4b0 */
FixedVector vToTarget; /* 0x0059d4d0 */
short asCannedCommand[16]; /* 0x0059d4e0 */
unsigned short ausVduModeStack[16]; /* 0x0059d500 */
char acShipSequence[10]; /* 0x0059d520 */
short nTargetFacing; /* 0x0059d52a */
FixedVector aShipDestination[10]; /* 0x0059d530 */
short aasShipShield[10][2]; /* 0x0059d5b0 */
enum ShipTactic aeShipTactic[12]; /* 0x0059d5e0 */
signed char abShipExhaustHeat[10]; /* 0x0059d610 */
short nRenderedSpaceFrame; /* 0x0059d61a */
short nEyePitchGoal; /* 0x0059d61c */
signed char acShipStress[16]; /* 0x0059d620 */
short anRollGoal[16]; /* 0x0059d630 */
enum Side aeShipSide[12]; /* 0x0059d650 */
signed char acShipAiCooldown[16]; /* 0x0059d680 */
FixedVector vCollisionDelta; /* 0x0059d690 */
signed char acLastCollisionObject[SPACE_OBJECT_COUNT]; /* 0x0059d6a0 */
short aasShipMaximumShield[10][2]; /* 0x0059d6e0 */
short asObjectCollisionRadius[64]; /* 0x0059d710 */
signed char acShipPointingMode[16]; /* 0x0059d790 */
short anPitchGoal[16]; /* 0x0059d7a0 */
signed char abShipNavPointIndex[16]; /* 0x0059d7c0 */
signed char acTurnInterval[16]; /* 0x0059d7d0 */
short anObjectRollRotation[SPACE_OBJECT_COUNT]; /* 0x0059d7e0 */
signed char abShipTurn[16]; /* 0x0059d860 */
HazardField aHazardFields[7]; /* 0x0059d870 */
short nFacingToTarget; /* 0x0059d920 */
short asObjectScreenY[SPACE_OBJECT_COUNT]; /* 0x0059d930 */
short asObjectScreenX[SPACE_OBJECT_COUNT]; /* 0x0059d9b0 */
short asObjectAnimationIndex[SPACE_OBJECT_COUNT]; /* 0x0059da30 */
signed char cCockpitView; /* 0x0059dab0 */
MissionObjective aMissionObjectives[MISSION_OBJECTIVE_COUNT]; /* 0x0059dac0 */
enum ShipManeuver aeShipManeuver[12]; /* 0x0059dcb0 */
const short *apCannedSequence[12]; /* 0x0059dce0 */
FixedVector aShipMissionSpot[10]; /* 0x0059dd10 */
short asCapitalShipViewFrame[16]; /* 0x0059dd90 */
signed char acObjectCollisionGraceTicks[ /* 0x0059ddb0 */
    SPACE_OBJECT_COUNT];
ObjectResourceSlot aObjectResourceSlots[4]; /* 0x0059ddf0 */
signed char acShipDestroyedWeaponCount[16]; /* 0x0059de30 */
unsigned short awObjectScale[SPACE_OBJECT_COUNT]; /* 0x0059de40 */
signed char acVduModeStackDepth[16]; /* 0x0059dec0 */
signed char acShipSpawnNavPoint[16]; /* 0x0059ded0 */
short asShipAccumulatedDamage[SPACE_OBJECT_COUNT]; /* 0x0059dee0 */
short nCurrentNavPoint; /* 0x0059df60 */
int nJoystickCalibrationMinimumY; /* 0x0059df64 */
int nJoystickCalibrationMinimumX; /* 0x0059df68 */
int nJoystickCalibrationMaximumX; /* 0x0059df6c */
int nJoystickCalibrationMaximumY; /* 0x0059df70 */
unsigned short awPaletteEntryAllocation[256]; /* 0x0059df80 */
unsigned char *apCommPortraitShapes[16]; /* 0x0059e180 */
char szHudMessageBuffer[0x60]; /* 0x0059e1c0 */
char *aapszPilotSpeech[14][11]; /* 0x0059e220 */
signed char abCommMenuChoiceCommand[7]; /* 0x0059e488 */
const char *pszCommMenuHeading; /* 0x0059e490 */
char szCommMenuHeadingBuffer[0x40]; /* 0x0059e4a0 */
const char *apszCommMenuChoiceText[7]; /* 0x0059e4e0 */
int anInputButton2PressTime[2]; /* 0x0059e500 */
short asInputButton1DoubleClick[2]; /* 0x0059e508 */
short asInputButton1Changed[2]; /* 0x0059e50c */
short asInputButton2Changed[2]; /* 0x0059e510 */
short asPreviousInputButtons[2]; /* 0x0059e514 */
int anInputButton1PressTime[2]; /* 0x0059e518 */
short asInputButton2DoubleClick[2]; /* 0x0059e520 */
void *apPacketHandles[0x1000]; /* 0x0059e530 */
int aiPacketHandleOffsets[0x1000]; /* 0x005a2530 */
int nPacketHandleCount; /* 0x005a6530 */
Viewport *pAllocatedScreenViewport; /* 0x005a6534 */
int *DAT_005a6538;
unsigned short awScreenRowOffsets[202]; /* 0x005a6540 */
int nScreenAllocationState; /* 0x005a66e0 */
Viewport *pAllocatedScreenViewportMirror; /* 0x005a66e4 */
int aiSoundEffectSourceActive[0x41]; /* 0x005a66ec */
unsigned char abSoundPlaybackSlots[0xf8]; /* 0x005a67f0 */
int nPassingShipSoundCooldown; /* 0x005a68e8 */
int nAllocateViewportCalls; /* 0x005a68ec */
void *pLastPacketAllocation; /* 0x005a68f0 */
FireworkState aFireworks[30]; /* 0x005a6900 */
unsigned char *pFireworkShape; /* 0x005a6a68 */
ConstellationStar aConstellationStars[10]; /* 0x005a6a70 */
Viewport *pConstellationViewport; /* 0x005a6aac */
short nConstellationStarCount; /* 0x005a6ab0 */
ConstellationParticle aConstellationParticles[16]; /* 0x005a6ac0 */
Viewport stConstellationViewport; /* 0x005a6b40 */
short nConstellationParticleCount; /* 0x005a6b54 */
const short *pViewScript; /* 0x005a6b58 */
short nExternalViewDistance; /* 0x005a6b5c */
Viewport stPilotHand; /* 0x005a6b60 */
Viewport stLeftVdu; /* 0x005a6b80 */
ScreenViewportPacket *pScreenViewportPacket; /* 0x005a6b94 */
Viewport stScreen; /* 0x005a6ba0 */
TextContext stHudMessageTextContext; /* 0x005a6bc0 */
Viewport stCockpitBar; /* 0x005a6be0 */
unsigned char *pTargetLockShape; /* 0x005a6bf4 */
unsigned char *apTextFonts[4]; /* 0x005a6c00 */
FontWorkspace **apFontWorkspaces[4]; /* 0x005a6c10 */
FixedVector aDirectionViewRightVector[DIRECTION_VIEW_COUNT]; /* 0x005a6c20 */
FixedVector aDirectionViewUpVector[DIRECTION_VIEW_COUNT]; /* 0x005a6f10 */
FixedVector aDirectionViewForwardVector[DIRECTION_VIEW_COUNT]; /* 0x005a7200 */
TextContext stLeftVduTextContext; /* 0x005a74f0 */
Viewport stSpaceBuffer; /* 0x005a7510 */
Viewport stRightVdu; /* 0x005a7530 */
Viewport stPilotHandBackdrop; /* 0x005a7550 */
unsigned char *pCockpitWeaponShape; /* 0x005a7564 */
Viewport stConversationTextViewport; /* 0x005a7570 */
char szDefaultTextBuffer[0xc8]; /* 0x005a7590 */
unsigned char *pCockpitIndicatorShape; /* 0x005a7658 */
unsigned char *pConstellationShape; /* 0x005a765c */
unsigned char *pCommMenuCursorShape; /* 0x005a7660 */
Viewport stModalSourceViewport; /* 0x005a7670 */
unsigned char *pPilotHandShape; /* 0x005a7684 */
Viewport stPilotHandComposite; /* 0x005a7690 */
Viewport stSceneBuffer; /* 0x005a76b0 */
short aPaletteFadeEntries[6][3]; /* 0x005a76d0 */
unsigned char *pCockpitDamageShape; /* 0x005a76f4 */
TextContext stRightVduTextContext; /* 0x005a7700 */
TextContext stCockpitReadoutTextContext; /* 0x005a7720 */
TextContext stDefaultTextContext; /* 0x005a7740 */
TextContext stConversationTextContext; /* 0x005a7760 */
short asDamageFlashColour[3]; /* 0x005a7780 */
short nDamageDisplayTicks; /* 0x005a7786 */
short nWeaponDisplayOriginX; /* 0x005a7788 */
short nWeaponDisplayOriginY; /* 0x005a778a */
char szDamageStatusText[0x48]; /* 0x005a7790 */
ShortPoint stDamageSpritePosition; /* 0x005a77d8 */
signed char cDamageDisplayComponent; /* 0x005a77dc */
signed char cDamageDisplayFrame; /* 0x005a77dd */
signed char cDamagedComponentCount; /* 0x005a77de */
int nDamageDisplayState; /* 0x005a77e0 */
int nDamageDisplayPhase; /* 0x005a77e4 */
unsigned char *pLaunchDoorShape; /* 0x005a77e8 */
unsigned int nFrameTimerId; /* 0x005a77ec */
unsigned char abPaletteTriplets[256][3]; /* 0x005a77f0 */
InputDeviceSample stPreviousFlightInput; /* 0x005a7af0 */
short bMouseButtonEventQueued; /* 0x005a7afc */
short bKeyboardEventQueued; /* 0x005a7afe */
short bMouseMoveEventQueued; /* 0x005a7b00 */
char *pStartupArguments[30]; /* 0x005a7b10 */
short bJoystickEventQueued; /* 0x005a7b88 */
short nArcadeStartupParameter; /* 0x005a7b8a */
Viewport stTrainSimTitleDisplayViewport; /* 0x005a7b90 */
Viewport stTrainSimHighScoreBufferViewport; /* 0x005a7bb0 */
int nArcadeScore; /* 0x005a7bc4 */
TextContext stTrainSimTextContext; /* 0x005a7bd0 */
Viewport stTrainSimHighScoreDisplayViewport; /* 0x005a7bf0 */
TextContext stTrainSimHighScoreTextContext; /* 0x005a7c10 */
short nArcadeTimeRemaining; /* 0x005a7c2c */
HighScoreEntry aHighScoreEntries[6]; /* 0x005a7c30 */
int nArcadeWaveBonus; /* 0x005a7c50 */
unsigned char *pSceneAnimationSceneData; /* 0x005a7c54 */
unsigned char *pSceneAnimationPrimaryShape; /* 0x005a7c58 */
unsigned char *pSceneAnimationTextData; /* 0x005a7c5c */
unsigned char *pSceneAnimationPacket; /* 0x005a7c60 */
SceneAnimationObject *pSceneAnimationObjects; /* 0x005a7c64 */
short nSceneAnimationWaitFrames; /* 0x005a7c68 */
unsigned char *pSceneAnimationDefinitions; /* 0x005a7c6c */
unsigned char *pSceneAnimationSecondaryShape; /* 0x005a7c70 */
signed char cCockpitLogicalFile; /* 0x005a7c74 */
unsigned char *apCockpitShapes[4]; /* 0x005a7c80 */
unsigned char *pCinematicViewBackdrop; /* 0x005a7c90 */
unsigned char *pRearViewBackdrop; /* 0x005a7c94 */
int bPlayerDestroyed; /* 0x005a7c98 */
short nPlayerKillCount; /* 0x005a7c9c */
short asCollisionTime[10]; /* 0x005a7ca0 */
int nWingmanKilledThisMission; /* 0x005a7cb4 */
short nWingmanKillCount; /* 0x005a7cb8 */
short nLastFoundShip; /* 0x005a7cba */
short asCollisionPartner[10]; /* 0x005a7cc0 */
int nMemoryConfiguration; /* 0x005a7cd4 */
volatile unsigned int dwOriginalFreeMemory; /* 0x005a7cd8 */
unsigned char *pMouseCursorResource; /* 0x005a7cdc */
int nAvailableGameMemory; /* 0x005a7ce0 */
int nSceneResourceBudget; /* 0x005a7ce4 */
int nAfterburnerSoundDeadline; /* 0x005a7ce8 */
int bAfterburnerSfxActive; /* 0x005a7cec */
DiskFileRecord *pDiskFileRecords; /* 0x005a7cf0 */
Viewport stDiskPromptBackgroundViewport; /* 0x005a7d00 */
unsigned char abDiskPromptDriveState[2]; /* 0x005a7d20 */
Viewport stDiskPromptViewport; /* 0x005a7d40 */
int bPointerMovedByKeyboard; /* 0x005a7d54 */
TextContext stDiskPromptTextContext; /* 0x005a7d60 */
ShortRect stDiskPromptBounds; /* 0x005a7d80 */
int DAT_005a7d9c;
signed char cCapitalShipLogicalFile; /* 0x005a7da0 */
FixedVector vNormalizedToTarget; /* 0x005a7db0 */
short nReleaseWeaponDisplayX; /* 0x005a7dbc */
short nReleaseWeaponDisplayY; /* 0x005a7dbe */
enum ObjectType eReleaseWeaponDisplayType; /* 0x005a7dc0 */
unsigned char *pScannerMarkerBackground; /* 0x005a7dc4 */
unsigned char bStickIndicatorFrame; /* 0x005a7dc8 */
short nMessageTimer; /* 0x005a7dca */
short nPendingCockpitDamage; /* 0x005a7dcc */
HudMessageSlot aHudMessageSlots[2]; /* 0x005a7dd0 */
signed char cPreviousTargetObject; /* 0x005a7df2 */
int nSavedMouseCursorY; /* 0x005a7df4 */
int nSavedMouseCursorX; /* 0x005a7df8 */
char szComponentHitMessage[40]; /* 0x005a7e00 */
short nTargetLockMarkerY; /* 0x005a7e28 */
short nCarrierMissionShipIndex; /* 0x005a7e2a */
CockpitReadout aCockpitReadouts[6]; /* 0x005a7e30 */
short nScannerCursorX; /* 0x005a7e6c */
short nScannerCursorY; /* 0x005a7e6e */
unsigned char abCockpitLightState[7]; /* 0x005a7e70 */
short asScannerObjectY[11]; /* 0x005a7e80 */
volatile ShortPoint stCockpitExplosionPosition; /* 0x005a7e98 */
short asScannerObjectX[11]; /* 0x005a7ea0 */
unsigned char abCockpitLightGoal[7]; /* 0x005a7eb8 */
int nDamageAlarmSfxHandle; /* 0x005a7ec0 */
short asScannerBackgroundColour[10]; /* 0x005a7ed0 */
unsigned int anCockpitDamageState[4]; /* 0x005a7ef0 */
unsigned short DAT_005a7f00;
short nStarFieldJRotation; /* 0x005a7f02 */
short nStarFieldIRotation; /* 0x005a7f04 */
short nExternalViewAngle; /* 0x005a7f06 */
unsigned char *pTitleShape; /* 0x005a7f08 */
int nViewportAllocationCount; /* 0x005a7f0c */
unsigned char *apViewportAllocations[128]; /* 0x005a7f10 */
int nFreeViewportCalls; /* 0x005a8110 */
short nPendingCampaignIndex; /* 0x005a8114 */
short nMissionMedalScore; /* 0x005a8116 */
short nCampaignDataSet; /* 0x005a8118 */
volatile int nNearHeapSize; /* 0x005a811c */
volatile int nNearHeapBase; /* 0x005a8120 */
int nNearHeapFirstDescriptor; /* 0x005a8124 */
void * volatile pNearHeapAllocation; /* 0x005a8128 */
short nNavMapMaximumX; /* 0x005a812c */
short nNavMapMinimumX; /* 0x005a812e */
unsigned short awNavObjectiveLabelIndex[16]; /* 0x005a8130 */
short nNavMapMaximumY; /* 0x005a8150 */
short nNavMapCentreX; /* 0x005a8152 */
short nNavMapMinimumY; /* 0x005a8154 */
TextContext stNavMapTextContext; /* 0x005a8160 */
short nNavMapCentreY; /* 0x005a817c */
TextContext stNavLabelTextContext; /* 0x005a8180 */
short nActiveInputDevice; /* 0x005a819c */
int nJoystickVerticalDeadZone; /* 0x005a81a0 */
int nJoystickHorizontalDeadZone; /* 0x005a81a4 */
int nJoystickUpScale; /* 0x005a81a8 */
int nJoystickLeftScale; /* 0x005a81ac */
int nJoystickMaximumX; /* 0x005a81b0 */
int nJoystickMaximumY; /* 0x005a81b4 */
int nJoystickMinimumX; /* 0x005a81b8 */
int nJoystickMinimumY; /* 0x005a81bc */
int nJoystickRawX; /* 0x005a81c0 */
int nJoystickRawY; /* 0x005a81c4 */
int nJoystickVerticalRange; /* 0x005a81c8 */
int nJoystickHorizontalRange; /* 0x005a81cc */
int nJoystickRightScale; /* 0x005a81d0 */
int nJoystickDownScale; /* 0x005a81d4 */
int nJoystickCentreY; /* 0x005a81d8 */
int nJoystickCentreX; /* 0x005a81dc */
int nJoystickFailureValue; /* 0x005a81e0 */
InputDeviceSample aInputDeviceSamples[2]; /* 0x005a81f0 */
short nMenuInputRepeatDelay; /* 0x005a8208 */
unsigned char abMissionAuxData[0x28]; /* 0x005a8210 */
unsigned char abSeriesAuxData[0x28]; /* 0x005a8240 */
MissionObjectiveSource aMissionObjectiveSources[16]; /* 0x005a8270 */
short nMissionEntryNavPoint; /* 0x005a8690 */
short nHomeMissionShipIndex; /* 0x005a8692 */
short nPlayerMissionShipIndex; /* 0x005a8694 */
short nInitialMissionShipIndices[8]; /* 0x005a8696 */
short DAT_005a86a6;
CampaignDate *pCurrentCampaignDate; /* 0x005a86a8 */
CampaignDate *pElapsedCampaignDate; /* 0x005a86ac */
signed char cObjectResourceLogicalFile; /* 0x005a86b0 */
Viewport *pScrambleViewport; /* 0x005a86b4 */
short nScrambleCanopyFrame; /* 0x005a86b8 */
unsigned char *pScrambleDetailShape; /* 0x005a86bc */
short nScrambleCockpitScale; /* 0x005a86c0 */
short nScrambleCockpitDetailX; /* 0x005a86c4 */
short nScrambleCockpitDetailY; /* 0x005a86c6 */
short nFuneralCasketX; /* 0x005a86c8 */
short nFuneralCasketY; /* 0x005a86ca */
short nScrambleShipDetailCount; /* 0x005a86cc */
signed char acScrambleShipDetailIndices[24]; /* 0x005a86d0 */
signed char cScrambleWalkTicks; /* 0x005a86e8 */
unsigned char *pConversationSpecialShape; /* 0x005a86ec */
ShortPoint aFuneralParticles[7]; /* 0x005a86f0 */
unsigned char *pScrambleBackgroundShape; /* 0x005a870c */
short nFuneralParticleDistance; /* 0x005a8710 */
short nScrambleBackgroundY; /* 0x005a8712 */
short nScrambleBackgroundRightX; /* 0x005a8714 */
short nFuneralForegroundX; /* 0x005a8718 */
short nFuneralForegroundY; /* 0x005a871a */
short nFuneralMainScale; /* 0x005a871c */
short nFuneralRifleFrame; /* 0x005a871e */
short nFuneralBaseX; /* 0x005a8720 */
short nFuneralBaseY; /* 0x005a8722 */
short nScrambleShipX; /* 0x005a8724 */
short nScrambleShipY; /* 0x005a8726 */
short nFuneralParticleScale; /* 0x005a8728 */
unsigned char *pScrambleHangarShape; /* 0x005a872c */
unsigned char *pScrambleCockpitShape; /* 0x005a8730 */
short nScrambleWalkerY; /* 0x005a8734 */
short nScrambleCanopyOffset; /* 0x005a8736 */
short nFuneralMainDistance; /* 0x005a8738 */
signed char bScrambleCanopyClosed; /* 0x005a873a */
short nFuneralGuardFrame; /* 0x005a873c */
short nScrambleOverlayX; /* 0x005a8740 */
short nScrambleOverlayY; /* 0x005a8742 */
unsigned char *pScrambleOverlayShape; /* 0x005a8744 */
signed char cScrambleWalkerPair; /* 0x005a8748 */
unsigned char *pScrambleCanopyShape; /* 0x005a874c */
unsigned char *pScrambleShipShape; /* 0x005a8750 */
short nTalkingHeadFaceX; /* 0x005a8754 */
short nTalkingHeadFaceY; /* 0x005a8756 */
short nTalkingHeadMouthY; /* 0x005a8758 */
short nTalkingHeadMouthX; /* 0x005a875a */
char szSoundDebugMessage[512]; /* 0x005a8760 */
unsigned char *pIntroFont; /* 0x005a8960 */
unsigned int nSystemKeyDown; /* 0x005a8964 */
JOYINFO aJoystickInfo[2]; /* 0x005a8970 */
int nHostMouseMessageX; /* 0x005a8990 */
int nHostMouseMessageY; /* 0x005a8994 */
int bHostPrimaryMouseButton; /* 0x005a8998 */
int bHostSecondaryMouseButton; /* 0x005a899c */
HWND hMainWindow; /* 0x005a89a0 */
HANDLE hSingleInstanceSemaphore; /* 0x005a89a4 */
unsigned long dwGameClockBase; /* 0x005a89a8 */
GuardedAllocation *pGuardedAllocationTail; /* 0x005a89ac */
char szMemoryUsage[0x80]; /* 0x005a89b0 */
HDC hMainWindowDC; /* 0x005a8a30 */
HCURSOR hPreviousCursor; /* 0x005a8a34 */
unsigned int nSessionEndTime; /* 0x005a8a38 */
int bMainWindowAlive; /* 0x005a8a3c */
HINSTANCE hAppInstance; /* 0x005a8a40 */
unsigned int nSessionStartTime; /* 0x005a8a44 */
unsigned short awPaletteRgbWords[0x300]; /* 0x005a8a50 */
