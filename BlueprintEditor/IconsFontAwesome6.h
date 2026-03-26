// IconsFontAwesome6.h -- Font Awesome 6 icon codepoints for ImGui
// Based on: https://github.com/juliettef/IconFontCppHeaders
// Font: fa-solid-900.ttf (Font Awesome 6 Free Solid)
#pragma once

#define FONT_ICON_FILE_NAME_FAS "fa-solid-900.ttf"

// Icon range for glyph loading
#define ICON_MIN_FA 0xe005
#define ICON_MAX_FA 0xf8ff

// =====================================================================
// Icon definitions used by Blueprint Editor
// Format: ICON_FA_<NAME> = UTF-8 encoded codepoint
// =====================================================================

// --- File operations ---
#define ICON_FA_FILE               "\xef\x85\x9b"  // U+f15b
#define ICON_FA_FILE_CIRCLE_PLUS   "\xef\x99\x8e"  // U+f65e  (or use ICON_FA_FILE_MEDICAL)
#define ICON_FA_FOLDER_OPEN        "\xef\x81\xbc"  // U+f07c
#define ICON_FA_FLOPPY_DISK        "\xef\x83\x87"  // U+f0c7  (save)
#define ICON_FA_FILE_EXPORT        "\xef\x95\xae"  // U+f56e  (save as)
#define ICON_FA_XMARK              "\xef\x80\x8d"  // U+f00d  (close)
#define ICON_FA_CLOCK_ROTATE_LEFT  "\xef\x87\x9a"  // U+f1da  (recent)

// --- Edit operations ---
#define ICON_FA_COPY               "\xef\x83\x85"  // U+f0c5
#define ICON_FA_PASTE              "\xef\x83\xaa"  // U+f0ea
#define ICON_FA_SCISSORS           "\xef\x83\x84"  // U+f0c4
#define ICON_FA_CLONE              "\xef\x89\x8d"  // U+f24d  (duplicate)
#define ICON_FA_OBJECT_GROUP       "\xef\x89\x87"  // U+f247  (select all)
#define ICON_FA_MAGNIFYING_GLASS   "\xef\x80\x82"  // U+f002  (search/find)
#define ICON_FA_TRASH              "\xef\x87\xb8"  // U+f1f8  (delete)
#define ICON_FA_TRASH_CAN          "\xef\x8b\xad"  // U+f2ed

// --- Align ---
#define ICON_FA_ALIGN_LEFT         "\xef\x80\xb6"  // U+f036
#define ICON_FA_ALIGN_RIGHT        "\xef\x80\xb8"  // U+f038
#define ICON_FA_ALIGN_CENTER       "\xef\x80\xb7"  // U+f037

// --- View / Navigation ---
#define ICON_FA_EXPAND             "\xef\x81\xa5"  // U+f065  (zoom to content)
#define ICON_FA_COMPRESS           "\xef\x81\xa6"  // U+f066
#define ICON_FA_LIST               "\xef\x80\xba"  // U+f03a  (node list)
#define ICON_FA_MAP                "\xef\x89\xb9"  // U+f279  (minimap)
#define ICON_FA_EYE                "\xef\x81\xae"  // U+f06e
#define ICON_FA_EYE_SLASH          "\xef\x81\xb0"  // U+f070
#define ICON_FA_PALETTE            "\xef\x94\xbf"  // U+f53f  (style editor)
#define ICON_FA_BARS               "\xef\x83\x89"  // U+f0c9
#define ICON_FA_TABLE_CELLS        "\xef\x80\x8a"  // U+f00a  (ordinals/grid)
#define ICON_FA_SITEMAP            "\xef\x83\xa8"  // U+f0e8

// --- Execution / Run ---
#define ICON_FA_PLAY               "\xef\x81\x8b"  // U+f04b
#define ICON_FA_STOP               "\xef\x81\x8d"  // U+f04d
#define ICON_FA_CIRCLE_PLAY        "\xef\x85\x84"  // U+f144
#define ICON_FA_CIRCLE_STOP        "\xef\x84\x8d"  // U+f28d
#define ICON_FA_BOLT               "\xef\x83\xa7"  // U+f0e7  (execute)
#define ICON_FA_TERMINAL           "\xef\x84\xa0"  // U+f120  (output)
#define ICON_FA_BUG                "\xef\x86\x88"  // U+f188  (debug)

// --- Status / Feedback ---
#define ICON_FA_CIRCLE_CHECK       "\xef\x81\x98"  // U+f058  (success)
#define ICON_FA_CIRCLE_XMARK       "\xef\x81\x97"  // U+f057  (error)
#define ICON_FA_TRIANGLE_EXCLAMATION "\xef\x81\xb1" // U+f071 (warning)
#define ICON_FA_CIRCLE_INFO        "\xef\x81\x9a"  // U+f05a  (info)
#define ICON_FA_CIRCLE_EXCLAMATION "\xef\x81\xaa"  // U+f06a

// --- Timer ---
#define ICON_FA_CLOCK              "\xef\x80\x97"  // U+f017
#define ICON_FA_STOPWATCH          "\xef\x8b\xb2"  // U+f2f2
#define ICON_FA_GAUGE_HIGH         "\xef\x98\xa5"  // U+f625
#define ICON_FA_PAUSE              "\xef\x81\x8c"  // U+f04c

// --- Panel headers / Structure ---
#define ICON_FA_CUBE               "\xef\x86\xb2"  // U+f1b2  (node)
#define ICON_FA_CUBES              "\xef\x86\xb3"  // U+f1b3  (nodes)
#define ICON_FA_DIAGRAM_PROJECT    "\xef\x95\x82"  // U+f542  (blueprint)
#define ICON_FA_CIRCLE_NODES       "\xe4\xab\x8e"  // U+e4ae  (nodes graph)
#define ICON_FA_CODE_BRANCH        "\xef\x84\xa6"  // U+f126
#define ICON_FA_LINK               "\xef\x83\x81"  // U+f0c1
#define ICON_FA_UNLINK             "\xef\x84\xa7"  // U+f127  (break link)
#define ICON_FA_LAYER_GROUP        "\xef\x97\x82"  // U+f5fd

// --- Collapse / Expand ---
#define ICON_FA_CARET_DOWN         "\xef\x83\x97"  // U+f0d7
#define ICON_FA_CARET_RIGHT        "\xef\x83\x9a"  // U+f0da

// --- Misc ---
#define ICON_FA_PLUS               "\x2b"           // U+002b
#define ICON_FA_MINUS              "\xef\x81\xa8"  // U+f068
#define ICON_FA_CIRCLE_PLUS        "\xef\x81\x95"  // U+f055
#define ICON_FA_CIRCLE_MINUS       "\xef\x81\x96"  // U+f056
#define ICON_FA_GEAR               "\xef\x80\x93"  // U+f013  (settings)
#define ICON_FA_GEARS              "\xef\x82\x85"  // U+f085
#define ICON_FA_SLIDERS            "\xef\x87\x9e"  // U+f1de
#define ICON_FA_WAND_MAGIC_SPARKLES "\xee\x8b\x8b" // U+e2cb
#define ICON_FA_ARROW_RIGHT        "\xef\x81\xa1"  // U+f061
#define ICON_FA_ARROW_LEFT         "\xef\x81\xa0"  // U+f060
#define ICON_FA_ARROW_UP           "\xef\x81\xa2"  // U+f062
#define ICON_FA_ARROW_DOWN         "\xef\x81\xa3"  // U+f063
#define ICON_FA_ARROWS_ROTATE      "\xef\x80\xa1"  // U+f021  (refresh/redo)
#define ICON_FA_ARROW_ROTATE_LEFT  "\xef\x83\xa2"  // U+f0e2  (undo)
#define ICON_FA_DOWNLOAD           "\xef\x80\x99"  // U+f019
#define ICON_FA_UPLOAD             "\xef\x82\x93"  // U+f093
#define ICON_FA_QUESTION           "\x3f"           // U+003f
#define ICON_FA_CIRCLE_QUESTION    "\xef\x81\x99"  // U+f059
#define ICON_FA_KEYBOARD           "\xef\x84\x9c"  // U+f11c  (shortcuts)
#define ICON_FA_STAR               "\xef\x80\x85"  // U+f005
#define ICON_FA_ERASER             "\xef\x84\xad"  // U+f12d  (clear)
#define ICON_FA_GRIP_LINES         "\xef\x9e\xa4"  // U+f7a4  (splitter)
#define ICON_FA_CHECK              "\xef\x80\x8c"  // U+f00c
#define ICON_FA_PEN                "\xef\x8c\x84"  // U+f304
#define ICON_FA_PENCIL             "\xef\x8c\x83"  // U+f303
#define ICON_FA_ROTATE             "\xef\x8b\xb1"  // U+f2f1
#define ICON_FA_UP_DOWN_LEFT_RIGHT "\xef\x81\x87"  // U+f047  (move)
#define ICON_FA_HAND_POINTER       "\xef\x89\x9a"  // U+f25a
