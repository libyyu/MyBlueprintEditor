class_name VersionProject

var _projectName: String
var _verLatest: VersionElement
var _verBase: VersionElement
var _verMin: VersionElement
var _bCheckVerBase: bool
var _verPairs: Array[VersionElementPair]
var _bLoaded: bool

func _init():
	_projectName = ""
	_verLatest = VersionElement.new()
	_verBase = VersionElement.new()
	_verMin = VersionElement.new()
	_bCheckVerBase = true
	_verPairs = []
	_bLoaded = false

func SetCheckVerBase(bCheck: bool) -> void:
	_bCheckVerBase = bCheck

func IsCheckVerBase() -> bool:
	return _bCheckVerBase

func SetProjectName(name: String) -> void:
	_projectName = name

func GetProjectName() -> String:
	return _projectName

func SetVerLatest(ver: VersionElement) -> void:
	_verLatest = ver

func GetVerLatest() -> VersionElement:
	return _verLatest

func SetVerBase(ver: VersionElement) -> void:
	_verBase = ver

func GetVerBase() -> VersionElement:
	return _verBase

func SetVerMin(ver: VersionElement) -> void:
	_verMin = ver

func GetVerMin() -> VersionElement:
	return _verMin

func Clear() -> void:
	_verLatest = VersionElement.new()
	_verBase = VersionElement.new()
	_verMin = VersionElement.new()
	_bCheckVerBase = true
	_verPairs = []
	_bLoaded = false

func Load(content) -> bool:
	Clear()
	var json = JSON.new()
	var parse_error = json.parse(content)
	if parse_error != OK:
		push_error("json parse error")
		return false
	
	var data = json.data
	if typeof(data) != TYPE_DICTIONARY:
		push_error("json format not valid")
		return false
	
	var name: String = data.get("project_name", "")
	if name.is_empty():
		push_error("project_name not found")
		return false
	SetProjectName(name)
	
	var basever: VersionElement = VersionElement.parseFromString(data.get("base_version", "0.0.0"))
	if not basever.isValid():
		push_error("base_version not valid")
		return false	
	SetVerBase(basever)
	
	var minver: VersionElement = VersionElement.parseFromString(data.get("min_version", "0.0.0"))
	if not minver.isValid():
		push_error("min_version not valid")
		return false
	SetVerMin(minver)
	
	var latestver: VersionElement = VersionElement.parseFromString(data.get("latest_version","0.0.0"))
	if not latestver.isValid():
		push_error("latest_version not valid")
	SetVerLatest(latestver)
	
	var patches = data.get("patches", [])
	var n: int = patches.size()
	for i in range(n):
		var patch: Dictionary = patches[i]
		var elementPair: VersionElementPair = VersionElementPair.new()
		elementPair.VerFrom = VersionElement.parseFromString(patch.get("from", "0,0,0"))
		elementPair.VerTo = VersionElement.parseFromString(patch.get("to", "0,0,0"))
		elementPair.MD5 = patch.get("md5", "")
		elementPair.Size = int(patch.get("size", "0"))
		_verPairs.append(elementPair)
		
	_verPairs.sort_custom(func(l: VersionElementPair, r: VersionElementPair):
		if l.VerFrom.isEqual(r.VerFrom):
			return l.VerFrom.isLess(r.VerFrom)
		else:
			return l.VerTo.isLess(r.VerTo)
	)
	
	_bLoaded = true
	return true

func LoadFromFile(path: String) -> bool:
	return false

func IsLoaded() -> bool:
	return _bLoaded

func FindVersionPair(ver: VersionElement) -> VersionElementPair:
	if (not _verPairs or _verPairs.size() == 0):
		return null
	if ver.isEqual(_verLatest):
		return null
	if (_bCheckVerBase and ver.isLess(_verBase)):
		return null
	if not (ver.isLess(_verLatest)):
		return null

	var iVerOld: int = -1
	var verOld: VersionElement = VersionElement.new()
	var n := _verPairs.size()
	for i in range(n):
		var pair: VersionElementPair = _verPairs[i]
		var verFrom: VersionElement = pair.VerFrom
		if verFrom.isEqual(ver):
			iVerOld = i
			verOld = verFrom
			break
		elif verFrom.isLess(ver) and verFrom.isGreater(verOld):
			iVerOld = i
			verOld = verFrom
			
	if iVerOld < 0:
		return null
		
	var iVer: int = -1
	var verNew: VersionElement = VersionElement.new(0, 0, 0)
	for i in range(n):
		var pair: VersionElementPair = _verPairs[i]
		if pair.VerFrom.isEqual(verOld):
			var verTo: VersionElement = pair.VerTo
			if verTo.isGreater(verNew):
				iVer = i
				verNew = verTo
	
	if iVer >= 0:
		return _verPairs[iVer]
	
	return null

class VersionPairResult:
	var valid: bool = false
	var size: int = 0
	var verions: Array[VersionElementPair] = []

func CalcSize(verFrom: VersionElement, verTo: VersionElement) -> VersionPairResult:
	var result: VersionPairResult = VersionPairResult.new()
	if not (verFrom.isLess(verTo)): return result
	
	var nextPair: VersionElementPair = FindVersionPair(verFrom)
	if not nextPair: return result
	
	result.verions.append(nextPair)
	result.size = nextPair.Size
	while true:
		if not (verTo.isGreater(nextPair.VerTo)):
			break
		var _nextPair: VersionElementPair = FindVersionPair(nextPair.VerTo)	
		if not _nextPair or _nextPair == nextPair:
			break
		nextPair = _nextPair
		result.size += nextPair.Size
		result.verions.append(nextPair)
	
	result.valid = true
	return result
