class_name VersionElement
extends RefCounted

var v1: int
var v2: int
var v3: int

func _init(v1_: int = -1, v2_: int = -1, v3_: int = -1) -> void:
	setVersion(v1_, v2_, v3_)

static func parseFromString(version: String) -> VersionElement:
	var element = VersionElement.new()
	var parts = version.split(".")
	element.v1 = int(parts[0])
	element.v2 = int(parts[1])
	element.v3 = int(parts[2])
	return element
	
func setVersion(v1_: int = -1, v2_: int = -1, v3_: int = -1) -> void:
	v1 = v1_
	v2 = v2_
	v3 = v3_

func toString() -> String:
	return "%d.%d.%d" % [v1, v2, v3]

func resetToDefault() -> void:
	v1 = -1
	v2 = -1
	v3 = -1

func isDefault() -> bool:
	return v1 == -1 and v2 == -1 and v3 == -1

func isValid() -> bool:
	return v1 >= 0 and v2 >= 0 and v3 >= 0

func isEqual(other: VersionElement) -> bool:
	return v1 == other.v1 and v2 == other.v2 and v3 == other.v3

func isLess(other: VersionElement) -> bool:
	if v1 < other.v1:
		return true
	elif v1 == other.v1:
		if v2 < other.v2:
			return true
		elif v2 == other.v2:
			return v3 < other.v3
	return false

func isGreater(other: VersionElement) -> bool:
	if v1 > other.v1:
		return true
	elif v1 == other.v1:
		if v2 > other.v2:
			return true
		elif v2 == other.v2:
			return v3 > other.v3
	return false

func _to_string() -> String:
	return toString()
	
func _eq(other: VersionElement) -> bool:
	return isEqual(other)
	
func _lt(other: VersionElement) -> bool:
	return isLess(other)

func _gt(other: VersionElement) -> bool:
	return isGreater(other)

func _le(other: VersionElement) -> bool:
	return isLess(other) or isEqual(other)

func _ge(other: VersionElement) -> bool:
	return isGreater(other) or isEqual(other)

func _add(other: VersionElement) -> VersionElement:
	return VersionElement.new(v1 + other.v1, v2 + other.v2, v3 + other.v3)

func _sub(other: VersionElement) -> VersionElement:
	return VersionElement.new(v1 - other.v1, v2 - other.v2, v3 - other.v3)
