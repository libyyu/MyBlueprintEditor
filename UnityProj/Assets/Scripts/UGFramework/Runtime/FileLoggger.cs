using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;


namespace UGFramework.Runtime
{
    [DisallowMultipleComponent]
    [UnityEngine.Scripting.Preserve]
    public class FileLoggger : MonoBehaviour
    {
        #region logfile path
#if UNITY_EDITOR || !UNITY_WEBGL
        private const string LogFileName = "gamelog.log";
        private string CachePath
        {
            get
            {
#if true //UNITY_EDITOR || DEBUG
                return Application.dataPath + "/../Cache";
#else
            return Path.Combine(Application.persistentDataPath, "Cache");
#endif
            }
        }

        string LogPath
        {
            get { return Path.Combine(CachePath, "logs"); }
        }
#endif
        #endregion

        // Start is called before the first frame update
        void Awake()
        {
#if UNITY_EDITOR || !UNITY_WEBGL
            Application.logMessageReceived += LogCallback;
            var logFilePath = Path.Combine(LogPath, LogFileName);
            if (File.Exists(logFilePath))
                File.Delete(logFilePath);

            if (Directory.Exists(LogPath) == false)
                Directory.CreateDirectory(LogPath);

            using (File.Create(logFilePath))
            {

            }
            Debug.Log("logFilePath:" + logFilePath);
#endif
            Debug.Log("streamingAssetsPath:" + Application.streamingAssetsPath);
            Debug.Log("dataPath:" + Application.dataPath);
            Debug.Log("absoluteURL:" + Application.absoluteURL);
            Debug.Log("persistentDataPath:" + Application.persistentDataPath);
            Debug.Log("temporaryCachePath:" + Application.temporaryCachePath);
        }

        void OnDestroy()
        {
#if UNITY_EDITOR || !UNITY_WEBGL
            Application.logMessageReceived -= LogCallback;
#endif
        }

#if UNITY_EDITOR || !UNITY_WEBGL
        void LogCallback(string condition, string stackTrace, LogType type)
        {
            switch (type)
            {
                case LogType.Error:
                    WriteToFile(string.Format("[error]{0}:{1}", condition, stackTrace));
                    break;
                case LogType.Exception:
                    WriteToFile(string.Format("[exception]{0}:{1}", condition, stackTrace));
                    break;
                case LogType.Assert:
                    WriteToFile(string.Format("[assert]{0}", condition));
                    break;
                case LogType.Log:
                    WriteToFile(string.Format("[info]{0}", condition));
                    break;
                case LogType.Warning:
                    WriteToFile(string.Format("[warning]{0}", condition));
                    break;
            }
        }

        void WriteToFile(string message)
        {
            string time = "[" + System.DateTime.Now.ToString("yyyyMMddHHmmss") + "]";
            using (StreamWriter sw = File.AppendText(Path.Combine(LogPath, LogFileName)))
            {
                sw.WriteLine(time + "-" + message);
                sw.Flush();
            }
        }
#endif


    }
}