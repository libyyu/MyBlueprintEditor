mergeInto(LibraryManager.library, {
  logToWeb:function(str) 
  {
      console.log(UTF8ToString(str)); 
  },
  isRunEnvWX:function()
  {
    return typeof wx === "object";
  },
  callJSMethod:function(method, jsonStr)
  {
    var M;
    if (typeof GameGlobal === "object") {
      M = GameGlobal;
    }
    else {
      M = unityFramework;
    }
    if(typeof M === "object" && typeof M.callJSMethod === 'function'){
      var m = UTF8ToString(method);
      var a = UTF8ToString(jsonStr);
      var ret = M.callJSMethod(m, a);
      if(typeof ret === 'string') {
        var length = lengthBytesUTF8(ret) + 1;
        var buffer = _malloc(length);
        stringToUTF8(ret, buffer, length);
        return buffer;
      }
    } else {
      console.log(UTF8ToString(method) , " method not found");
    }
    return null;
  },
  freeStringArrayInJS:function(buffer)
  {
    _free(buffer);
  }
});