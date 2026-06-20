// UIToolkitExtensions.cs
// xLua 无法直接访问 InlineStyleAccess struct 的属性 setter/getter，
// 此类提供 C# 侧的桥接方法，供 Lua 通过 CS.UIToolkitExtensions 调用。
//
// 覆盖 IStyle 全部 82 个属性，每个属性 4 个方法（VisualElement set/get + IStyle set/get）。
// 命名规则：set_xxx / get_xxx，与 Lua 风格一致。
// 用法（Lua）：
//   CS.UIToolkitExtensions.set_position(ve, CS.UnityEngine.UIElements.Position.Absolute)
//   local w = CS.UIToolkitExtensions.get_width(ve)
//
using UnityEngine.UIElements;
using XLua;

[LuaCallCSharp]
public static class UIToolkitExtensions
{
    // ═══════════════════════════════════════════════════════════════
    // alignContent : Align
    // ═══════════════════════════════════════════════════════════════
    public static Align get_alignContent(VisualElement e) => e.style.alignContent.value;
    public static Align get_alignContent(IStyle s) => s.alignContent.value;
    public static void set_alignContent(VisualElement e, StyleEnum<Align> v) => e.style.alignContent = v;
    public static void set_alignContent(IStyle s, StyleEnum<Align> v) => s.alignContent = v;

    // ═══════════════════════════════════════════════════════════════
    // alignItems : Align
    // ═══════════════════════════════════════════════════════════════
    public static Align get_alignItems(VisualElement e) => e.style.alignItems.value;
    public static Align get_alignItems(IStyle s) => s.alignItems.value;
    public static void set_alignItems(VisualElement e, StyleEnum<Align> v) => e.style.alignItems = v;
    public static void set_alignItems(IStyle s, StyleEnum<Align> v) => s.alignItems = v;

    // ═══════════════════════════════════════════════════════════════
    // alignSelf : Align
    // ═══════════════════════════════════════════════════════════════
    public static Align get_alignSelf(VisualElement e) => e.style.alignSelf.value;
    public static Align get_alignSelf(IStyle s) => s.alignSelf.value;
    public static void set_alignSelf(VisualElement e, StyleEnum<Align> v) => e.style.alignSelf = v;
    public static void set_alignSelf(IStyle s, StyleEnum<Align> v) => s.alignSelf = v;

    // ═══════════════════════════════════════════════════════════════
    // backgroundColor : StyleColor
    // ═══════════════════════════════════════════════════════════════
    public static StyleColor get_backgroundColor(VisualElement e) => e.style.backgroundColor;
    public static StyleColor get_backgroundColor(IStyle s) => s.backgroundColor;
    public static void set_backgroundColor(VisualElement e, StyleColor v) => e.style.backgroundColor = v;
    public static void set_backgroundColor(IStyle s, StyleColor v) => s.backgroundColor = v;

    // ═══════════════════════════════════════════════════════════════
    // backgroundImage : StyleBackground
    // ═══════════════════════════════════════════════════════════════
    public static StyleBackground get_backgroundImage(VisualElement e) => e.style.backgroundImage;
    public static StyleBackground get_backgroundImage(IStyle s) => s.backgroundImage;
    public static void set_backgroundImage(VisualElement e, StyleBackground v) => e.style.backgroundImage = v;
    public static void set_backgroundImage(IStyle s, StyleBackground v) => s.backgroundImage = v;

    // ═══════════════════════════════════════════════════════════════
    // backgroundPositionX : StyleBackgroundPosition
    // ═══════════════════════════════════════════════════════════════
    public static StyleBackgroundPosition get_backgroundPositionX(VisualElement e) => e.style.backgroundPositionX;
    public static StyleBackgroundPosition get_backgroundPositionX(IStyle s) => s.backgroundPositionX;
    public static void set_backgroundPositionX(VisualElement e, StyleBackgroundPosition v) => e.style.backgroundPositionX = v;
    public static void set_backgroundPositionX(IStyle s, StyleBackgroundPosition v) => s.backgroundPositionX = v;

    // ═══════════════════════════════════════════════════════════════
    // backgroundPositionY : StyleBackgroundPosition
    // ═══════════════════════════════════════════════════════════════
    public static StyleBackgroundPosition get_backgroundPositionY(VisualElement e) => e.style.backgroundPositionY;
    public static StyleBackgroundPosition get_backgroundPositionY(IStyle s) => s.backgroundPositionY;
    public static void set_backgroundPositionY(VisualElement e, StyleBackgroundPosition v) => e.style.backgroundPositionY = v;
    public static void set_backgroundPositionY(IStyle s, StyleBackgroundPosition v) => s.backgroundPositionY = v;

    // ═══════════════════════════════════════════════════════════════
    // backgroundRepeat : StyleBackgroundRepeat
    // ═══════════════════════════════════════════════════════════════
    public static StyleBackgroundRepeat get_backgroundRepeat(VisualElement e) => e.style.backgroundRepeat;
    public static StyleBackgroundRepeat get_backgroundRepeat(IStyle s) => s.backgroundRepeat;
    public static void set_backgroundRepeat(VisualElement e, StyleBackgroundRepeat v) => e.style.backgroundRepeat = v;
    public static void set_backgroundRepeat(IStyle s, StyleBackgroundRepeat v) => s.backgroundRepeat = v;

    // ═══════════════════════════════════════════════════════════════
    // backgroundSize : StyleBackgroundSize
    // ═══════════════════════════════════════════════════════════════
    public static StyleBackgroundSize get_backgroundSize(VisualElement e) => e.style.backgroundSize;
    public static StyleBackgroundSize get_backgroundSize(IStyle s) => s.backgroundSize;
    public static void set_backgroundSize(VisualElement e, StyleBackgroundSize v) => e.style.backgroundSize = v;
    public static void set_backgroundSize(IStyle s, StyleBackgroundSize v) => s.backgroundSize = v;

    // ═══════════════════════════════════════════════════════════════
    // borderBottomColor : StyleColor
    // ═══════════════════════════════════════════════════════════════
    public static StyleColor get_borderBottomColor(VisualElement e) => e.style.borderBottomColor;
    public static StyleColor get_borderBottomColor(IStyle s) => s.borderBottomColor;
    public static void set_borderBottomColor(VisualElement e, StyleColor v) => e.style.borderBottomColor = v;
    public static void set_borderBottomColor(IStyle s, StyleColor v) => s.borderBottomColor = v;

    // ═══════════════════════════════════════════════════════════════
    // borderBottomLeftRadius : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_borderBottomLeftRadius(VisualElement e) => e.style.borderBottomLeftRadius;
    public static StyleLength get_borderBottomLeftRadius(IStyle s) => s.borderBottomLeftRadius;
    public static void set_borderBottomLeftRadius(VisualElement e, StyleLength v) => e.style.borderBottomLeftRadius = v;
    public static void set_borderBottomLeftRadius(IStyle s, StyleLength v) => s.borderBottomLeftRadius = v;

    // ═══════════════════════════════════════════════════════════════
    // borderBottomRightRadius : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_borderBottomRightRadius(VisualElement e) => e.style.borderBottomRightRadius;
    public static StyleLength get_borderBottomRightRadius(IStyle s) => s.borderBottomRightRadius;
    public static void set_borderBottomRightRadius(VisualElement e, StyleLength v) => e.style.borderBottomRightRadius = v;
    public static void set_borderBottomRightRadius(IStyle s, StyleLength v) => s.borderBottomRightRadius = v;

    // ═══════════════════════════════════════════════════════════════
    // borderBottomWidth : StyleFloat
    // ═══════════════════════════════════════════════════════════════
    public static StyleFloat get_borderBottomWidth(VisualElement e) => e.style.borderBottomWidth;
    public static StyleFloat get_borderBottomWidth(IStyle s) => s.borderBottomWidth;
    public static void set_borderBottomWidth(VisualElement e, StyleFloat v) => e.style.borderBottomWidth = v;
    public static void set_borderBottomWidth(IStyle s, StyleFloat v) => s.borderBottomWidth = v;

    // ═══════════════════════════════════════════════════════════════
    // borderLeftColor : StyleColor
    // ═══════════════════════════════════════════════════════════════
    public static StyleColor get_borderLeftColor(VisualElement e) => e.style.borderLeftColor;
    public static StyleColor get_borderLeftColor(IStyle s) => s.borderLeftColor;
    public static void set_borderLeftColor(VisualElement e, StyleColor v) => e.style.borderLeftColor = v;
    public static void set_borderLeftColor(IStyle s, StyleColor v) => s.borderLeftColor = v;

    // ═══════════════════════════════════════════════════════════════
    // borderLeftWidth : StyleFloat
    // ═══════════════════════════════════════════════════════════════
    public static StyleFloat get_borderLeftWidth(VisualElement e) => e.style.borderLeftWidth;
    public static StyleFloat get_borderLeftWidth(IStyle s) => s.borderLeftWidth;
    public static void set_borderLeftWidth(VisualElement e, StyleFloat v) => e.style.borderLeftWidth = v;
    public static void set_borderLeftWidth(IStyle s, StyleFloat v) => s.borderLeftWidth = v;

    // ═══════════════════════════════════════════════════════════════
    // borderRightColor : StyleColor
    // ═══════════════════════════════════════════════════════════════
    public static StyleColor get_borderRightColor(VisualElement e) => e.style.borderRightColor;
    public static StyleColor get_borderRightColor(IStyle s) => s.borderRightColor;
    public static void set_borderRightColor(VisualElement e, StyleColor v) => e.style.borderRightColor = v;
    public static void set_borderRightColor(IStyle s, StyleColor v) => s.borderRightColor = v;

    // ═══════════════════════════════════════════════════════════════
    // borderRightWidth : StyleFloat
    // ═══════════════════════════════════════════════════════════════
    public static StyleFloat get_borderRightWidth(VisualElement e) => e.style.borderRightWidth;
    public static StyleFloat get_borderRightWidth(IStyle s) => s.borderRightWidth;
    public static void set_borderRightWidth(VisualElement e, StyleFloat v) => e.style.borderRightWidth = v;
    public static void set_borderRightWidth(IStyle s, StyleFloat v) => s.borderRightWidth = v;

    // ═══════════════════════════════════════════════════════════════
    // borderTopColor : StyleColor
    // ═══════════════════════════════════════════════════════════════
    public static StyleColor get_borderTopColor(VisualElement e) => e.style.borderTopColor;
    public static StyleColor get_borderTopColor(IStyle s) => s.borderTopColor;
    public static void set_borderTopColor(VisualElement e, StyleColor v) => e.style.borderTopColor = v;
    public static void set_borderTopColor(IStyle s, StyleColor v) => s.borderTopColor = v;

    // ═══════════════════════════════════════════════════════════════
    // borderTopLeftRadius : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_borderTopLeftRadius(VisualElement e) => e.style.borderTopLeftRadius;
    public static StyleLength get_borderTopLeftRadius(IStyle s) => s.borderTopLeftRadius;
    public static void set_borderTopLeftRadius(VisualElement e, StyleLength v) => e.style.borderTopLeftRadius = v;
    public static void set_borderTopLeftRadius(IStyle s, StyleLength v) => s.borderTopLeftRadius = v;

    // ═══════════════════════════════════════════════════════════════
    // borderTopRightRadius : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_borderTopRightRadius(VisualElement e) => e.style.borderTopRightRadius;
    public static StyleLength get_borderTopRightRadius(IStyle s) => s.borderTopRightRadius;
    public static void set_borderTopRightRadius(VisualElement e, StyleLength v) => e.style.borderTopRightRadius = v;
    public static void set_borderTopRightRadius(IStyle s, StyleLength v) => s.borderTopRightRadius = v;

    // ═══════════════════════════════════════════════════════════════
    // borderTopWidth : StyleFloat
    // ═══════════════════════════════════════════════════════════════
    public static StyleFloat get_borderTopWidth(VisualElement e) => e.style.borderTopWidth;
    public static StyleFloat get_borderTopWidth(IStyle s) => s.borderTopWidth;
    public static void set_borderTopWidth(VisualElement e, StyleFloat v) => e.style.borderTopWidth = v;
    public static void set_borderTopWidth(IStyle s, StyleFloat v) => s.borderTopWidth = v;

    // ═══════════════════════════════════════════════════════════════
    // bottom : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_bottom(VisualElement e) => e.style.bottom;
    public static StyleLength get_bottom(IStyle s) => s.bottom;
    public static void set_bottom(VisualElement e, StyleLength v) => e.style.bottom = v;
    public static void set_bottom(IStyle s, StyleLength v) => s.bottom = v;

    // ═══════════════════════════════════════════════════════════════
    // color : StyleColor
    // ═══════════════════════════════════════════════════════════════
    public static StyleColor get_color(VisualElement e) => e.style.color;
    public static StyleColor get_color(IStyle s) => s.color;
    public static void set_color(VisualElement e, StyleColor v) => e.style.color = v;
    public static void set_color(IStyle s, StyleColor v) => s.color = v;

    // ═══════════════════════════════════════════════════════════════
    // cursor : StyleCursor
    // ═══════════════════════════════════════════════════════════════
    public static StyleCursor get_cursor(VisualElement e) => e.style.cursor;
    public static StyleCursor get_cursor(IStyle s) => s.cursor;
    public static void set_cursor(VisualElement e, StyleCursor v) => e.style.cursor = v;
    public static void set_cursor(IStyle s, StyleCursor v) => s.cursor = v;

    // ═══════════════════════════════════════════════════════════════
    // display : DisplayStyle
    // ═══════════════════════════════════════════════════════════════
    public static DisplayStyle get_display(VisualElement e) => e.style.display.value;
    public static DisplayStyle get_display(IStyle s) => s.display.value;
    public static void set_display(VisualElement e, StyleEnum<DisplayStyle> v) => e.style.display = v;
    public static void set_display(IStyle s, StyleEnum<DisplayStyle> v) => s.display = v;

    // ═══════════════════════════════════════════════════════════════
    // flexBasis : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_flexBasis(VisualElement e) => e.style.flexBasis;
    public static StyleLength get_flexBasis(IStyle s) => s.flexBasis;
    public static void set_flexBasis(VisualElement e, StyleLength v) => e.style.flexBasis = v;
    public static void set_flexBasis(IStyle s, StyleLength v) => s.flexBasis = v;

    // ═══════════════════════════════════════════════════════════════
    // flexDirection : FlexDirection
    // ═══════════════════════════════════════════════════════════════
    public static FlexDirection get_flexDirection(VisualElement e) => e.style.flexDirection.value;
    public static FlexDirection get_flexDirection(IStyle s) => s.flexDirection.value;
    public static void set_flexDirection(VisualElement e, StyleEnum<FlexDirection> v) => e.style.flexDirection = v;
    public static void set_flexDirection(IStyle s, StyleEnum<FlexDirection> v) => s.flexDirection = v;

    // ═══════════════════════════════════════════════════════════════
    // flexGrow : StyleFloat
    // ═══════════════════════════════════════════════════════════════
    public static StyleFloat get_flexGrow(VisualElement e) => e.style.flexGrow;
    public static StyleFloat get_flexGrow(IStyle s) => s.flexGrow;
    public static void set_flexGrow(VisualElement e, StyleFloat v) => e.style.flexGrow = v;
    public static void set_flexGrow(IStyle s, StyleFloat v) => s.flexGrow = v;

    // ═══════════════════════════════════════════════════════════════
    // flexShrink : StyleFloat
    // ═══════════════════════════════════════════════════════════════
    public static StyleFloat get_flexShrink(VisualElement e) => e.style.flexShrink;
    public static StyleFloat get_flexShrink(IStyle s) => s.flexShrink;
    public static void set_flexShrink(VisualElement e, StyleFloat v) => e.style.flexShrink = v;
    public static void set_flexShrink(IStyle s, StyleFloat v) => s.flexShrink = v;

    // ═══════════════════════════════════════════════════════════════
    // flexWrap : Wrap
    // ═══════════════════════════════════════════════════════════════
    public static Wrap get_flexWrap(VisualElement e) => e.style.flexWrap.value;
    public static Wrap get_flexWrap(IStyle s) => s.flexWrap.value;
    public static void set_flexWrap(VisualElement e, StyleEnum<Wrap> v) => e.style.flexWrap = v;
    public static void set_flexWrap(IStyle s, StyleEnum<Wrap> v) => s.flexWrap = v;

    // ═══════════════════════════════════════════════════════════════
    // fontSize : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_fontSize(VisualElement e) => e.style.fontSize;
    public static StyleLength get_fontSize(IStyle s) => s.fontSize;
    public static void set_fontSize(VisualElement e, StyleLength v) => e.style.fontSize = v;
    public static void set_fontSize(IStyle s, StyleLength v) => s.fontSize = v;

    // ═══════════════════════════════════════════════════════════════
    // height : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_height(VisualElement e) => e.style.height;
    public static StyleLength get_height(IStyle s) => s.height;
    public static void set_height(VisualElement e, StyleLength v) => e.style.height = v;
    public static void set_height(IStyle s, StyleLength v) => s.height = v;

    // ═══════════════════════════════════════════════════════════════
    // justifyContent : Justify
    // ═══════════════════════════════════════════════════════════════
    public static Justify get_justifyContent(VisualElement e) => e.style.justifyContent.value;
    public static Justify get_justifyContent(IStyle s) => s.justifyContent.value;
    public static void set_justifyContent(VisualElement e, StyleEnum<Justify> v) => e.style.justifyContent = v;
    public static void set_justifyContent(IStyle s, StyleEnum<Justify> v) => s.justifyContent = v;

    // ═══════════════════════════════════════════════════════════════
    // left : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_left(VisualElement e) => e.style.left;
    public static StyleLength get_left(IStyle s) => s.left;
    public static void set_left(VisualElement e, StyleLength v) => e.style.left = v;
    public static void set_left(IStyle s, StyleLength v) => s.left = v;

    // ═══════════════════════════════════════════════════════════════
    // letterSpacing : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_letterSpacing(VisualElement e) => e.style.letterSpacing;
    public static StyleLength get_letterSpacing(IStyle s) => s.letterSpacing;
    public static void set_letterSpacing(VisualElement e, StyleLength v) => e.style.letterSpacing = v;
    public static void set_letterSpacing(IStyle s, StyleLength v) => s.letterSpacing = v;

    // ═══════════════════════════════════════════════════════════════
    // marginBottom : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_marginBottom(VisualElement e) => e.style.marginBottom;
    public static StyleLength get_marginBottom(IStyle s) => s.marginBottom;
    public static void set_marginBottom(VisualElement e, StyleLength v) => e.style.marginBottom = v;
    public static void set_marginBottom(IStyle s, StyleLength v) => s.marginBottom = v;

    // ═══════════════════════════════════════════════════════════════
    // marginLeft : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_marginLeft(VisualElement e) => e.style.marginLeft;
    public static StyleLength get_marginLeft(IStyle s) => s.marginLeft;
    public static void set_marginLeft(VisualElement e, StyleLength v) => e.style.marginLeft = v;
    public static void set_marginLeft(IStyle s, StyleLength v) => s.marginLeft = v;

    // ═══════════════════════════════════════════════════════════════
    // marginRight : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_marginRight(VisualElement e) => e.style.marginRight;
    public static StyleLength get_marginRight(IStyle s) => s.marginRight;
    public static void set_marginRight(VisualElement e, StyleLength v) => e.style.marginRight = v;
    public static void set_marginRight(IStyle s, StyleLength v) => s.marginRight = v;

    // ═══════════════════════════════════════════════════════════════
    // marginTop : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_marginTop(VisualElement e) => e.style.marginTop;
    public static StyleLength get_marginTop(IStyle s) => s.marginTop;
    public static void set_marginTop(VisualElement e, StyleLength v) => e.style.marginTop = v;
    public static void set_marginTop(IStyle s, StyleLength v) => s.marginTop = v;

    // ═══════════════════════════════════════════════════════════════
    // maxHeight : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_maxHeight(VisualElement e) => e.style.maxHeight;
    public static StyleLength get_maxHeight(IStyle s) => s.maxHeight;
    public static void set_maxHeight(VisualElement e, StyleLength v) => e.style.maxHeight = v;
    public static void set_maxHeight(IStyle s, StyleLength v) => s.maxHeight = v;

    // ═══════════════════════════════════════════════════════════════
    // maxWidth : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_maxWidth(VisualElement e) => e.style.maxWidth;
    public static StyleLength get_maxWidth(IStyle s) => s.maxWidth;
    public static void set_maxWidth(VisualElement e, StyleLength v) => e.style.maxWidth = v;
    public static void set_maxWidth(IStyle s, StyleLength v) => s.maxWidth = v;

    // ═══════════════════════════════════════════════════════════════
    // minHeight : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_minHeight(VisualElement e) => e.style.minHeight;
    public static StyleLength get_minHeight(IStyle s) => s.minHeight;
    public static void set_minHeight(VisualElement e, StyleLength v) => e.style.minHeight = v;
    public static void set_minHeight(IStyle s, StyleLength v) => s.minHeight = v;

    // ═══════════════════════════════════════════════════════════════
    // minWidth : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_minWidth(VisualElement e) => e.style.minWidth;
    public static StyleLength get_minWidth(IStyle s) => s.minWidth;
    public static void set_minWidth(VisualElement e, StyleLength v) => e.style.minWidth = v;
    public static void set_minWidth(IStyle s, StyleLength v) => s.minWidth = v;

    // ═══════════════════════════════════════════════════════════════
    // opacity : StyleFloat
    // ═══════════════════════════════════════════════════════════════
    public static StyleFloat get_opacity(VisualElement e) => e.style.opacity;
    public static StyleFloat get_opacity(IStyle s) => s.opacity;
    public static void set_opacity(VisualElement e, StyleFloat v) => e.style.opacity = v;
    public static void set_opacity(IStyle s, StyleFloat v) => s.opacity = v;

    // ═══════════════════════════════════════════════════════════════
    // overflow : Overflow
    // ═══════════════════════════════════════════════════════════════
    public static Overflow get_overflow(VisualElement e) => e.style.overflow.value;
    public static Overflow get_overflow(IStyle s) => s.overflow.value;
    public static void set_overflow(VisualElement e, StyleEnum<Overflow> v) => e.style.overflow = v;
    public static void set_overflow(IStyle s, StyleEnum<Overflow> v) => s.overflow = v;

    // ═══════════════════════════════════════════════════════════════
    // paddingBottom : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_paddingBottom(VisualElement e) => e.style.paddingBottom;
    public static StyleLength get_paddingBottom(IStyle s) => s.paddingBottom;
    public static void set_paddingBottom(VisualElement e, StyleLength v) => e.style.paddingBottom = v;
    public static void set_paddingBottom(IStyle s, StyleLength v) => s.paddingBottom = v;

    // ═══════════════════════════════════════════════════════════════
    // paddingLeft : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_paddingLeft(VisualElement e) => e.style.paddingLeft;
    public static StyleLength get_paddingLeft(IStyle s) => s.paddingLeft;
    public static void set_paddingLeft(VisualElement e, StyleLength v) => e.style.paddingLeft = v;
    public static void set_paddingLeft(IStyle s, StyleLength v) => s.paddingLeft = v;

    // ═══════════════════════════════════════════════════════════════
    // paddingRight : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_paddingRight(VisualElement e) => e.style.paddingRight;
    public static StyleLength get_paddingRight(IStyle s) => s.paddingRight;
    public static void set_paddingRight(VisualElement e, StyleLength v) => e.style.paddingRight = v;
    public static void set_paddingRight(IStyle s, StyleLength v) => s.paddingRight = v;

    // ═══════════════════════════════════════════════════════════════
    // paddingTop : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_paddingTop(VisualElement e) => e.style.paddingTop;
    public static StyleLength get_paddingTop(IStyle s) => s.paddingTop;
    public static void set_paddingTop(VisualElement e, StyleLength v) => e.style.paddingTop = v;
    public static void set_paddingTop(IStyle s, StyleLength v) => s.paddingTop = v;

    // ═══════════════════════════════════════════════════════════════
    // position : Position
    // ═══════════════════════════════════════════════════════════════
    public static Position get_position(VisualElement e) => e.style.position.value;
    public static Position get_position(IStyle s) => s.position.value;
    public static void set_position(VisualElement e, StyleEnum<Position> v) => e.style.position = v;
    public static void set_position(IStyle s, StyleEnum<Position> v) => s.position = v;

    // convenience: set_position(Position) for Lua enum usage
    public static void set_position(VisualElement e, Position v) => e.style.position = v;
    public static void set_position(IStyle s, Position v) => s.position = v;

    // ═══════════════════════════════════════════════════════════════
    // right : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_right(VisualElement e) => e.style.right;
    public static StyleLength get_right(IStyle s) => s.right;
    public static void set_right(VisualElement e, StyleLength v) => e.style.right = v;
    public static void set_right(IStyle s, StyleLength v) => s.right = v;

    // ═══════════════════════════════════════════════════════════════
    // rotate : StyleRotate
    // ═══════════════════════════════════════════════════════════════
    public static StyleRotate get_rotate(VisualElement e) => e.style.rotate;
    public static StyleRotate get_rotate(IStyle s) => s.rotate;
    public static void set_rotate(VisualElement e, StyleRotate v) => e.style.rotate = v;
    public static void set_rotate(IStyle s, StyleRotate v) => s.rotate = v;

    // ═══════════════════════════════════════════════════════════════
    // scale : StyleScale
    // ═══════════════════════════════════════════════════════════════
    public static StyleScale get_scale(VisualElement e) => e.style.scale;
    public static StyleScale get_scale(IStyle s) => s.scale;
    public static void set_scale(VisualElement e, StyleScale v) => e.style.scale = v;
    public static void set_scale(IStyle s, StyleScale v) => s.scale = v;

    // ═══════════════════════════════════════════════════════════════
    // textOverflow : TextOverflow
    // ═══════════════════════════════════════════════════════════════
    public static TextOverflow get_textOverflow(VisualElement e) => e.style.textOverflow.value;
    public static TextOverflow get_textOverflow(IStyle s) => s.textOverflow.value;
    public static void set_textOverflow(VisualElement e, StyleEnum<TextOverflow> v) => e.style.textOverflow = v;
    public static void set_textOverflow(IStyle s, StyleEnum<TextOverflow> v) => s.textOverflow = v;

    // ═══════════════════════════════════════════════════════════════
    // textShadow : StyleTextShadow
    // ═══════════════════════════════════════════════════════════════
    public static StyleTextShadow get_textShadow(VisualElement e) => e.style.textShadow;
    public static StyleTextShadow get_textShadow(IStyle s) => s.textShadow;
    public static void set_textShadow(VisualElement e, StyleTextShadow v) => e.style.textShadow = v;
    public static void set_textShadow(IStyle s, StyleTextShadow v) => s.textShadow = v;

    // ═══════════════════════════════════════════════════════════════
    // top : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_top(VisualElement e) => e.style.top;
    public static StyleLength get_top(IStyle s) => s.top;
    public static void set_top(VisualElement e, StyleLength v) => e.style.top = v;
    public static void set_top(IStyle s, StyleLength v) => s.top = v;

    // ═══════════════════════════════════════════════════════════════
    // transformOrigin : StyleTransformOrigin
    // ═══════════════════════════════════════════════════════════════
    public static StyleTransformOrigin get_transformOrigin(VisualElement e) => e.style.transformOrigin;
    public static StyleTransformOrigin get_transformOrigin(IStyle s) => s.transformOrigin;
    public static void set_transformOrigin(VisualElement e, StyleTransformOrigin v) => e.style.transformOrigin = v;
    public static void set_transformOrigin(IStyle s, StyleTransformOrigin v) => s.transformOrigin = v;

    // ═══════════════════════════════════════════════════════════════
    // transitionDelay : StyleList<TimeValue>
    // ═══════════════════════════════════════════════════════════════
    public static StyleList<TimeValue> get_transitionDelay(VisualElement e) => e.style.transitionDelay;
    public static StyleList<TimeValue> get_transitionDelay(IStyle s) => s.transitionDelay;
    public static void set_transitionDelay(VisualElement e, StyleList<TimeValue> v) => e.style.transitionDelay = v;
    public static void set_transitionDelay(IStyle s, StyleList<TimeValue> v) => s.transitionDelay = v;

    // ═══════════════════════════════════════════════════════════════
    // transitionDuration : StyleList<TimeValue>
    // ═══════════════════════════════════════════════════════════════
    public static StyleList<TimeValue> get_transitionDuration(VisualElement e) => e.style.transitionDuration;
    public static StyleList<TimeValue> get_transitionDuration(IStyle s) => s.transitionDuration;
    public static void set_transitionDuration(VisualElement e, StyleList<TimeValue> v) => e.style.transitionDuration = v;
    public static void set_transitionDuration(IStyle s, StyleList<TimeValue> v) => s.transitionDuration = v;

    // ═══════════════════════════════════════════════════════════════
    // transitionProperty : StyleList<StylePropertyName>
    // ═══════════════════════════════════════════════════════════════
    public static StyleList<StylePropertyName> get_transitionProperty(VisualElement e) => e.style.transitionProperty;
    public static StyleList<StylePropertyName> get_transitionProperty(IStyle s) => s.transitionProperty;
    public static void set_transitionProperty(VisualElement e, StyleList<StylePropertyName> v) => e.style.transitionProperty = v;
    public static void set_transitionProperty(IStyle s, StyleList<StylePropertyName> v) => s.transitionProperty = v;

    // ═══════════════════════════════════════════════════════════════
    // transitionTimingFunction : StyleList<EasingFunction>
    // ═══════════════════════════════════════════════════════════════
    public static StyleList<EasingFunction> get_transitionTimingFunction(VisualElement e) => e.style.transitionTimingFunction;
    public static StyleList<EasingFunction> get_transitionTimingFunction(IStyle s) => s.transitionTimingFunction;
    public static void set_transitionTimingFunction(VisualElement e, StyleList<EasingFunction> v) => e.style.transitionTimingFunction = v;
    public static void set_transitionTimingFunction(IStyle s, StyleList<EasingFunction> v) => s.transitionTimingFunction = v;

    // ═══════════════════════════════════════════════════════════════
    // translate : StyleTranslate
    // ═══════════════════════════════════════════════════════════════
    public static StyleTranslate get_translate(VisualElement e) => e.style.translate;
    public static StyleTranslate get_translate(IStyle s) => s.translate;
    public static void set_translate(VisualElement e, StyleTranslate v) => e.style.translate = v;
    public static void set_translate(IStyle s, StyleTranslate v) => s.translate = v;

    // ═══════════════════════════════════════════════════════════════
    // unityBackgroundImageTintColor : StyleColor
    // ═══════════════════════════════════════════════════════════════
    public static StyleColor get_unityBackgroundImageTintColor(VisualElement e) => e.style.unityBackgroundImageTintColor;
    public static StyleColor get_unityBackgroundImageTintColor(IStyle s) => s.unityBackgroundImageTintColor;
    public static void set_unityBackgroundImageTintColor(VisualElement e, StyleColor v) => e.style.unityBackgroundImageTintColor = v;
    public static void set_unityBackgroundImageTintColor(IStyle s, StyleColor v) => s.unityBackgroundImageTintColor = v;

    // ═══════════════════════════════════════════════════════════════
    // unityFont : StyleFont
    // ═══════════════════════════════════════════════════════════════
    public static StyleFont get_unityFont(VisualElement e) => e.style.unityFont;
    public static StyleFont get_unityFont(IStyle s) => s.unityFont;
    public static void set_unityFont(VisualElement e, StyleFont v) => e.style.unityFont = v;
    public static void set_unityFont(IStyle s, StyleFont v) => s.unityFont = v;

    // ═══════════════════════════════════════════════════════════════
    // unityFontDefinition : StyleFontDefinition
    // ═══════════════════════════════════════════════════════════════
    public static StyleFontDefinition get_unityFontDefinition(VisualElement e) => e.style.unityFontDefinition;
    public static StyleFontDefinition get_unityFontDefinition(IStyle s) => s.unityFontDefinition;
    public static void set_unityFontDefinition(VisualElement e, StyleFontDefinition v) => e.style.unityFontDefinition = v;
    public static void set_unityFontDefinition(IStyle s, StyleFontDefinition v) => s.unityFontDefinition = v;

    // ═══════════════════════════════════════════════════════════════
    // unityFontStyleAndWeight : FontStyle
    // ═══════════════════════════════════════════════════════════════
    public static UnityEngine.FontStyle get_unityFontStyleAndWeight(VisualElement e) => e.style.unityFontStyleAndWeight.value;
    public static UnityEngine.FontStyle get_unityFontStyleAndWeight(IStyle s) => s.unityFontStyleAndWeight.value;
    public static void set_unityFontStyleAndWeight(VisualElement e, StyleEnum<UnityEngine.FontStyle> v) => e.style.unityFontStyleAndWeight = v;
    public static void set_unityFontStyleAndWeight(IStyle s, StyleEnum<UnityEngine.FontStyle> v) => s.unityFontStyleAndWeight = v;

    // ═══════════════════════════════════════════════════════════════
    // unityOverflowClipBox : OverflowClipBox
    // ═══════════════════════════════════════════════════════════════
    public static OverflowClipBox get_unityOverflowClipBox(VisualElement e) => e.style.unityOverflowClipBox.value;
    public static OverflowClipBox get_unityOverflowClipBox(IStyle s) => s.unityOverflowClipBox.value;
    public static void set_unityOverflowClipBox(VisualElement e, StyleEnum<OverflowClipBox> v) => e.style.unityOverflowClipBox = v;
    public static void set_unityOverflowClipBox(IStyle s, StyleEnum<OverflowClipBox> v) => s.unityOverflowClipBox = v;

    // ═══════════════════════════════════════════════════════════════
    // unityParagraphSpacing : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_unityParagraphSpacing(VisualElement e) => e.style.unityParagraphSpacing;
    public static StyleLength get_unityParagraphSpacing(IStyle s) => s.unityParagraphSpacing;
    public static void set_unityParagraphSpacing(VisualElement e, StyleLength v) => e.style.unityParagraphSpacing = v;
    public static void set_unityParagraphSpacing(IStyle s, StyleLength v) => s.unityParagraphSpacing = v;

    // ═══════════════════════════════════════════════════════════════
    // unitySliceBottom : StyleInt
    // ═══════════════════════════════════════════════════════════════
    public static StyleInt get_unitySliceBottom(VisualElement e) => e.style.unitySliceBottom;
    public static StyleInt get_unitySliceBottom(IStyle s) => s.unitySliceBottom;
    public static void set_unitySliceBottom(VisualElement e, StyleInt v) => e.style.unitySliceBottom = v;
    public static void set_unitySliceBottom(IStyle s, StyleInt v) => s.unitySliceBottom = v;

    // ═══════════════════════════════════════════════════════════════
    // unitySliceLeft : StyleInt
    // ═══════════════════════════════════════════════════════════════
    public static StyleInt get_unitySliceLeft(VisualElement e) => e.style.unitySliceLeft;
    public static StyleInt get_unitySliceLeft(IStyle s) => s.unitySliceLeft;
    public static void set_unitySliceLeft(VisualElement e, StyleInt v) => e.style.unitySliceLeft = v;
    public static void set_unitySliceLeft(IStyle s, StyleInt v) => s.unitySliceLeft = v;

    // ═══════════════════════════════════════════════════════════════
    // unitySliceRight : StyleInt
    // ═══════════════════════════════════════════════════════════════
    public static StyleInt get_unitySliceRight(VisualElement e) => e.style.unitySliceRight;
    public static StyleInt get_unitySliceRight(IStyle s) => s.unitySliceRight;
    public static void set_unitySliceRight(VisualElement e, StyleInt v) => e.style.unitySliceRight = v;
    public static void set_unitySliceRight(IStyle s, StyleInt v) => s.unitySliceRight = v;

    // ═══════════════════════════════════════════════════════════════
    // unitySliceScale : StyleFloat
    // ═══════════════════════════════════════════════════════════════
    public static StyleFloat get_unitySliceScale(VisualElement e) => e.style.unitySliceScale;
    public static StyleFloat get_unitySliceScale(IStyle s) => s.unitySliceScale;
    public static void set_unitySliceScale(VisualElement e, StyleFloat v) => e.style.unitySliceScale = v;
    public static void set_unitySliceScale(IStyle s, StyleFloat v) => s.unitySliceScale = v;

    // ═══════════════════════════════════════════════════════════════
    // unitySliceTop : StyleInt
    // ═══════════════════════════════════════════════════════════════
    public static StyleInt get_unitySliceTop(VisualElement e) => e.style.unitySliceTop;
    public static StyleInt get_unitySliceTop(IStyle s) => s.unitySliceTop;
    public static void set_unitySliceTop(VisualElement e, StyleInt v) => e.style.unitySliceTop = v;
    public static void set_unitySliceTop(IStyle s, StyleInt v) => s.unitySliceTop = v;

    // ═══════════════════════════════════════════════════════════════
    // unityTextAlign : TextAnchor
    // ═══════════════════════════════════════════════════════════════
    public static UnityEngine.TextAnchor get_unityTextAlign(VisualElement e) => e.style.unityTextAlign.value;
    public static UnityEngine.TextAnchor get_unityTextAlign(IStyle s) => s.unityTextAlign.value;
    public static void set_unityTextAlign(VisualElement e, StyleEnum<UnityEngine.TextAnchor> v) => e.style.unityTextAlign = v;
    public static void set_unityTextAlign(IStyle s, StyleEnum<UnityEngine.TextAnchor> v) => s.unityTextAlign = v;

    // ═══════════════════════════════════════════════════════════════
    // unityTextOutlineColor : StyleColor
    // ═══════════════════════════════════════════════════════════════
    public static StyleColor get_unityTextOutlineColor(VisualElement e) => e.style.unityTextOutlineColor;
    public static StyleColor get_unityTextOutlineColor(IStyle s) => s.unityTextOutlineColor;
    public static void set_unityTextOutlineColor(VisualElement e, StyleColor v) => e.style.unityTextOutlineColor = v;
    public static void set_unityTextOutlineColor(IStyle s, StyleColor v) => s.unityTextOutlineColor = v;

    // ═══════════════════════════════════════════════════════════════
    // unityTextOutlineWidth : StyleFloat
    // ═══════════════════════════════════════════════════════════════
    public static StyleFloat get_unityTextOutlineWidth(VisualElement e) => e.style.unityTextOutlineWidth;
    public static StyleFloat get_unityTextOutlineWidth(IStyle s) => s.unityTextOutlineWidth;
    public static void set_unityTextOutlineWidth(VisualElement e, StyleFloat v) => e.style.unityTextOutlineWidth = v;
    public static void set_unityTextOutlineWidth(IStyle s, StyleFloat v) => s.unityTextOutlineWidth = v;

    // ═══════════════════════════════════════════════════════════════
    // unityTextOverflowPosition : TextOverflowPosition
    // ═══════════════════════════════════════════════════════════════
    public static TextOverflowPosition get_unityTextOverflowPosition(VisualElement e) => e.style.unityTextOverflowPosition.value;
    public static TextOverflowPosition get_unityTextOverflowPosition(IStyle s) => s.unityTextOverflowPosition.value;
    public static void set_unityTextOverflowPosition(VisualElement e, StyleEnum<TextOverflowPosition> v) => e.style.unityTextOverflowPosition = v;
    public static void set_unityTextOverflowPosition(IStyle s, StyleEnum<TextOverflowPosition> v) => s.unityTextOverflowPosition = v;

    // ═══════════════════════════════════════════════════════════════
    // visibility : Visibility
    // ═══════════════════════════════════════════════════════════════
    public static Visibility get_visibility(VisualElement e) => e.style.visibility.value;
    public static Visibility get_visibility(IStyle s) => s.visibility.value;
    public static void set_visibility(VisualElement e, StyleEnum<Visibility> v) => e.style.visibility = v;
    public static void set_visibility(IStyle s, StyleEnum<Visibility> v) => s.visibility = v;

    // convenience: set_visibility(Visibility) for Lua enum usage
    public static void set_visibility(VisualElement e, Visibility v) => e.style.visibility = v;
    public static void set_visibility(IStyle s, Visibility v) => s.visibility = v;

    // ═══════════════════════════════════════════════════════════════
    // whiteSpace : WhiteSpace
    // ═══════════════════════════════════════════════════════════════
    public static WhiteSpace get_whiteSpace(VisualElement e) => e.style.whiteSpace.value;
    public static WhiteSpace get_whiteSpace(IStyle s) => s.whiteSpace.value;
    public static void set_whiteSpace(VisualElement e, StyleEnum<WhiteSpace> v) => e.style.whiteSpace = v;
    public static void set_whiteSpace(IStyle s, StyleEnum<WhiteSpace> v) => s.whiteSpace = v;

    // ═══════════════════════════════════════════════════════════════
    // width : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_width(VisualElement e) => e.style.width;
    public static StyleLength get_width(IStyle s) => s.width;
    public static void set_width(VisualElement e, StyleLength v) => e.style.width = v;
    public static void set_width(IStyle s, StyleLength v) => s.width = v;

    // ═══════════════════════════════════════════════════════════════
    // wordSpacing : StyleLength
    // ═══════════════════════════════════════════════════════════════
    public static StyleLength get_wordSpacing(VisualElement e) => e.style.wordSpacing;
    public static StyleLength get_wordSpacing(IStyle s) => s.wordSpacing;
    public static void set_wordSpacing(VisualElement e, StyleLength v) => e.style.wordSpacing = v;
    public static void set_wordSpacing(IStyle s, StyleLength v) => s.wordSpacing = v;

    // ═══════════════════════════════════════════════════════════════
    // 以下为 StyleLength 属性的 convenience 重载（Length / float）
    // ═══════════════════════════════════════════════════════════════

    // borderBottomLeftRadius
    public static void set_borderBottomLeftRadius(VisualElement e, Length v)   => e.style.borderBottomLeftRadius = v;
    public static void set_borderBottomLeftRadius(IStyle s, Length v)          => s.borderBottomLeftRadius = v;
    public static void set_borderBottomLeftRadius(VisualElement e, float v)    => e.style.borderBottomLeftRadius = v;
    public static void set_borderBottomLeftRadius(IStyle s, float v)           => s.borderBottomLeftRadius = v;

    // borderBottomRightRadius
    public static void set_borderBottomRightRadius(VisualElement e, Length v)  => e.style.borderBottomRightRadius = v;
    public static void set_borderBottomRightRadius(IStyle s, Length v)         => s.borderBottomRightRadius = v;
    public static void set_borderBottomRightRadius(VisualElement e, float v)   => e.style.borderBottomRightRadius = v;
    public static void set_borderBottomRightRadius(IStyle s, float v)          => s.borderBottomRightRadius = v;

    // borderTopLeftRadius
    public static void set_borderTopLeftRadius(VisualElement e, Length v)      => e.style.borderTopLeftRadius = v;
    public static void set_borderTopLeftRadius(IStyle s, Length v)             => s.borderTopLeftRadius = v;
    public static void set_borderTopLeftRadius(VisualElement e, float v)       => e.style.borderTopLeftRadius = v;
    public static void set_borderTopLeftRadius(IStyle s, float v)              => s.borderTopLeftRadius = v;

    // borderTopRightRadius
    public static void set_borderTopRightRadius(VisualElement e, Length v)     => e.style.borderTopRightRadius = v;
    public static void set_borderTopRightRadius(IStyle s, Length v)            => s.borderTopRightRadius = v;
    public static void set_borderTopRightRadius(VisualElement e, float v)      => e.style.borderTopRightRadius = v;
    public static void set_borderTopRightRadius(IStyle s, float v)             => s.borderTopRightRadius = v;

    // bottom
    public static void set_bottom(VisualElement e, Length v)                   => e.style.bottom = v;
    public static void set_bottom(IStyle s, Length v)                          => s.bottom = v;
    public static void set_bottom(VisualElement e, float v)                    => e.style.bottom = v;
    public static void set_bottom(IStyle s, float v)                           => s.bottom = v;

    // flexBasis
    public static void set_flexBasis(VisualElement e, Length v)                => e.style.flexBasis = v;
    public static void set_flexBasis(IStyle s, Length v)                       => s.flexBasis = v;
    public static void set_flexBasis(VisualElement e, float v)                 => e.style.flexBasis = v;
    public static void set_flexBasis(IStyle s, float v)                        => s.flexBasis = v;

    // fontSize
    public static void set_fontSize(VisualElement e, Length v)                 => e.style.fontSize = v;
    public static void set_fontSize(IStyle s, Length v)                        => s.fontSize = v;
    public static void set_fontSize(VisualElement e, float v)                  => e.style.fontSize = v;
    public static void set_fontSize(IStyle s, float v)                         => s.fontSize = v;

    // height
    public static void set_height(VisualElement e, Length v)                   => e.style.height = v;
    public static void set_height(IStyle s, Length v)                          => s.height = v;
    public static void set_height(VisualElement e, float v)                    => e.style.height = v;
    public static void set_height(IStyle s, float v)                           => s.height = v;

    // left
    public static void set_left(VisualElement e, Length v)                     => e.style.left = v;
    public static void set_left(IStyle s, Length v)                            => s.left = v;
    public static void set_left(VisualElement e, float v)                      => e.style.left = v;
    public static void set_left(IStyle s, float v)                             => s.left = v;

    // letterSpacing
    public static void set_letterSpacing(VisualElement e, Length v)            => e.style.letterSpacing = v;
    public static void set_letterSpacing(IStyle s, Length v)                   => s.letterSpacing = v;
    public static void set_letterSpacing(VisualElement e, float v)             => e.style.letterSpacing = v;
    public static void set_letterSpacing(IStyle s, float v)                    => s.letterSpacing = v;

    // marginBottom
    public static void set_marginBottom(VisualElement e, Length v)             => e.style.marginBottom = v;
    public static void set_marginBottom(IStyle s, Length v)                    => s.marginBottom = v;
    public static void set_marginBottom(VisualElement e, float v)              => e.style.marginBottom = v;
    public static void set_marginBottom(IStyle s, float v)                     => s.marginBottom = v;

    // marginLeft
    public static void set_marginLeft(VisualElement e, Length v)               => e.style.marginLeft = v;
    public static void set_marginLeft(IStyle s, Length v)                      => s.marginLeft = v;
    public static void set_marginLeft(VisualElement e, float v)                => e.style.marginLeft = v;
    public static void set_marginLeft(IStyle s, float v)                       => s.marginLeft = v;

    // marginRight
    public static void set_marginRight(VisualElement e, Length v)              => e.style.marginRight = v;
    public static void set_marginRight(IStyle s, Length v)                     => s.marginRight = v;
    public static void set_marginRight(VisualElement e, float v)               => e.style.marginRight = v;
    public static void set_marginRight(IStyle s, float v)                      => s.marginRight = v;

    // marginTop
    public static void set_marginTop(VisualElement e, Length v)                => e.style.marginTop = v;
    public static void set_marginTop(IStyle s, Length v)                       => s.marginTop = v;
    public static void set_marginTop(VisualElement e, float v)                 => e.style.marginTop = v;
    public static void set_marginTop(IStyle s, float v)                        => s.marginTop = v;

    // maxHeight
    public static void set_maxHeight(VisualElement e, Length v)                => e.style.maxHeight = v;
    public static void set_maxHeight(IStyle s, Length v)                       => s.maxHeight = v;
    public static void set_maxHeight(VisualElement e, float v)                 => e.style.maxHeight = v;
    public static void set_maxHeight(IStyle s, float v)                        => s.maxHeight = v;

    // maxWidth
    public static void set_maxWidth(VisualElement e, Length v)                 => e.style.maxWidth = v;
    public static void set_maxWidth(IStyle s, Length v)                        => s.maxWidth = v;
    public static void set_maxWidth(VisualElement e, float v)                  => e.style.maxWidth = v;
    public static void set_maxWidth(IStyle s, float v)                         => s.maxWidth = v;

    // minHeight
    public static void set_minHeight(VisualElement e, Length v)                => e.style.minHeight = v;
    public static void set_minHeight(IStyle s, Length v)                       => s.minHeight = v;
    public static void set_minHeight(VisualElement e, float v)                 => e.style.minHeight = v;
    public static void set_minHeight(IStyle s, float v)                        => s.minHeight = v;

    // minWidth
    public static void set_minWidth(VisualElement e, Length v)                 => e.style.minWidth = v;
    public static void set_minWidth(IStyle s, Length v)                        => s.minWidth = v;
    public static void set_minWidth(VisualElement e, float v)                  => e.style.minWidth = v;
    public static void set_minWidth(IStyle s, float v)                         => s.minWidth = v;

    // paddingBottom
    public static void set_paddingBottom(VisualElement e, Length v)            => e.style.paddingBottom = v;
    public static void set_paddingBottom(IStyle s, Length v)                   => s.paddingBottom = v;
    public static void set_paddingBottom(VisualElement e, float v)             => e.style.paddingBottom = v;
    public static void set_paddingBottom(IStyle s, float v)                    => s.paddingBottom = v;

    // paddingLeft
    public static void set_paddingLeft(VisualElement e, Length v)              => e.style.paddingLeft = v;
    public static void set_paddingLeft(IStyle s, Length v)                     => s.paddingLeft = v;
    public static void set_paddingLeft(VisualElement e, float v)               => e.style.paddingLeft = v;
    public static void set_paddingLeft(IStyle s, float v)                      => s.paddingLeft = v;

    // paddingRight
    public static void set_paddingRight(VisualElement e, Length v)             => e.style.paddingRight = v;
    public static void set_paddingRight(IStyle s, Length v)                    => s.paddingRight = v;
    public static void set_paddingRight(VisualElement e, float v)              => e.style.paddingRight = v;
    public static void set_paddingRight(IStyle s, float v)                     => s.paddingRight = v;

    // paddingTop
    public static void set_paddingTop(VisualElement e, Length v)               => e.style.paddingTop = v;
    public static void set_paddingTop(IStyle s, Length v)                      => s.paddingTop = v;
    public static void set_paddingTop(VisualElement e, float v)                => e.style.paddingTop = v;
    public static void set_paddingTop(IStyle s, float v)                       => s.paddingTop = v;

    // right
    public static void set_right(VisualElement e, Length v)                    => e.style.right = v;
    public static void set_right(IStyle s, Length v)                           => s.right = v;
    public static void set_right(VisualElement e, float v)                     => e.style.right = v;
    public static void set_right(IStyle s, float v)                            => s.right = v;

    // top
    public static void set_top(VisualElement e, Length v)                      => e.style.top = v;
    public static void set_top(IStyle s, Length v)                             => s.top = v;
    public static void set_top(VisualElement e, float v)                       => e.style.top = v;
    public static void set_top(IStyle s, float v)                              => s.top = v;

    // unityParagraphSpacing
    public static void set_unityParagraphSpacing(VisualElement e, Length v)    => e.style.unityParagraphSpacing = v;
    public static void set_unityParagraphSpacing(IStyle s, Length v)           => s.unityParagraphSpacing = v;
    public static void set_unityParagraphSpacing(VisualElement e, float v)     => e.style.unityParagraphSpacing = v;
    public static void set_unityParagraphSpacing(IStyle s, float v)            => s.unityParagraphSpacing = v;

    // width
    public static void set_width(VisualElement e, Length v)                    => e.style.width = v;
    public static void set_width(IStyle s, Length v)                           => s.width = v;
    public static void set_width(VisualElement e, float v)                     => e.style.width = v;
    public static void set_width(IStyle s, float v)                            => s.width = v;

    // wordSpacing
    public static void set_wordSpacing(VisualElement e, Length v)              => e.style.wordSpacing = v;
    public static void set_wordSpacing(IStyle s, Length v)                     => s.wordSpacing = v;
    public static void set_wordSpacing(VisualElement e, float v)               => e.style.wordSpacing = v;
    public static void set_wordSpacing(IStyle s, float v)                      => s.wordSpacing = v;

    // color
    public static void set_color(VisualElement e, UnityEngine.Color v)  => e.style.color = v;
    public static void set_color(IStyle s, UnityEngine.Color v)         => s.color = v;

    // ═══════════════════════════════════════════════════════════════
    // 以下为 StyleEnum<T> 属性的 convenience 重载（裸枚举 T）
    // ═══════════════════════════════════════════════════════════════

    // alignContent : Align
    public static void set_alignContent(VisualElement e, Align v)  => e.style.alignContent = v;
    public static void set_alignContent(IStyle s, Align v)         => s.alignContent = v;

    // alignItems : Align
    public static void set_alignItems(VisualElement e, Align v)    => e.style.alignItems = v;
    public static void set_alignItems(IStyle s, Align v)           => s.alignItems = v;

    // alignSelf : Align
    public static void set_alignSelf(VisualElement e, Align v)     => e.style.alignSelf = v;
    public static void set_alignSelf(IStyle s, Align v)            => s.alignSelf = v;

    // display : DisplayStyle
    public static void set_display(VisualElement e, DisplayStyle v)  => e.style.display = v;
    public static void set_display(IStyle s, DisplayStyle v)         => s.display = v;

    // flexDirection : FlexDirection
    public static void set_flexDirection(VisualElement e, FlexDirection v)  => e.style.flexDirection = v;
    public static void set_flexDirection(IStyle s, FlexDirection v)         => s.flexDirection = v;

    // flexWrap : Wrap
    public static void set_flexWrap(VisualElement e, Wrap v)  => e.style.flexWrap = v;
    public static void set_flexWrap(IStyle s, Wrap v)         => s.flexWrap = v;

    // justifyContent : Justify
    public static void set_justifyContent(VisualElement e, Justify v)  => e.style.justifyContent = v;
    public static void set_justifyContent(IStyle s, Justify v)         => s.justifyContent = v;

    // overflow : Overflow
    public static void set_overflow(VisualElement e, Overflow v)  => e.style.overflow = v;
    public static void set_overflow(IStyle s, Overflow v)         => s.overflow = v;

    // textOverflow : TextOverflow
    public static void set_textOverflow(VisualElement e, TextOverflow v)  => e.style.textOverflow = v;
    public static void set_textOverflow(IStyle s, TextOverflow v)         => s.textOverflow = v;

    // unityFontStyleAndWeight : UnityEngine.FontStyle
    public static void set_unityFontStyleAndWeight(VisualElement e, UnityEngine.FontStyle v)  => e.style.unityFontStyleAndWeight = v;
    public static void set_unityFontStyleAndWeight(IStyle s, UnityEngine.FontStyle v)         => s.unityFontStyleAndWeight = v;

    // unityOverflowClipBox : OverflowClipBox
    public static void set_unityOverflowClipBox(VisualElement e, OverflowClipBox v)  => e.style.unityOverflowClipBox = v;
    public static void set_unityOverflowClipBox(IStyle s, OverflowClipBox v)         => s.unityOverflowClipBox = v;

    // unityTextAlign : UnityEngine.TextAnchor
    public static void set_unityTextAlign(VisualElement e, UnityEngine.TextAnchor v)  => e.style.unityTextAlign = v;
    public static void set_unityTextAlign(IStyle s, UnityEngine.TextAnchor v)         => s.unityTextAlign = v;

    // unityTextOverflowPosition : TextOverflowPosition
    public static void set_unityTextOverflowPosition(VisualElement e, TextOverflowPosition v)  => e.style.unityTextOverflowPosition = v;
    public static void set_unityTextOverflowPosition(IStyle s, TextOverflowPosition v)         => s.unityTextOverflowPosition = v;

    // whiteSpace : WhiteSpace
    public static void set_whiteSpace(VisualElement e, WhiteSpace v)  => e.style.whiteSpace = v;
    public static void set_whiteSpace(IStyle s, WhiteSpace v)         => s.whiteSpace = v;

}
