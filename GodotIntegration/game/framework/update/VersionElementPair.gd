class_name VersionElementPair
extends RefCounted

var VerFrom: VersionElement
var VerTo: VersionElement
var MD5: String
var Size: int

func _init() -> void:
	VerFrom = VersionElement.new()
	VerTo = VersionElement.new()
	MD5 = ""
	Size = 0
	
func toString() -> String:
	return "%s-%s %s %s" % [self.VerFrom.toString(), self.VerTo.toString(), self.md5, str(self.size)]

func GetPatcheFileName() -> String:
	return "%s-%s.pck" % [self.VerFrom.toString(), self.VerTo.toString()]
	
