#if UNITY_WEBGL && !UNITY_EDITOR
using UnityEngine;
using System.Runtime.InteropServices;
using System;
using System.Collections.Generic;

public class WebCommon
{
	[DllImport("__Internal")]
	public static extern void logToWeb(string str);

	[DllImport("__Internal")]
	public static extern bool isRunEnvWX();

    [DllImport("__Internal")]
    private static extern void freeStringArrayInJS(IntPtr buffer);
    
	[Serializable]
    private class Wrapper<T>
    {
        public T[] array;
    }
	private static string WrapArray(string jsonString)
    {
        return "{\"array\":" + jsonString + "}";
    }

    
	static Dictionary<string, string> querys = null;

    public static bool isDebugMode()
    {
		if (querys == null)
		{
            querys = new Dictionary<string, string>();
            var url = Application.absoluteURL;
			var query = new System.Uri(url).Query;
			query = query.TrimStart('?');
			// ½âÎö URL ²ÎÊý
			string[] parameters = query.Split('&');
			foreach (string parameter in parameters)
			{
				string[] keyValue = parameter.Split('=');
				string key = keyValue[0];
				string val = keyValue[1];
				if(querys.ContainsKey(key))
					querys[key] = val;
				else 
					querys.Add(key, val);
			}
		}

		string value;
		querys.TryGetValue("debug", out value);

        return value == "1";
    }
}

#endif