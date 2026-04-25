#if UNITY_2020_2_OR_NEWER
using UnityEditor.AssetImporters;
#else
using UnityEditor.Experimental.AssetImporters;
#endif

using UnityEngine;
using System.IO;

[ScriptedImporter(1, ".lua")]
public class LuaScriptImporter : ScriptedImporter
{
    public override void OnImportAsset(AssetImportContext ctx)
    {
        // ǿ�ƴ�ӡ��־
        Debug.Log($"{nameof(LuaScriptImporter)} Processed Import: {ctx.assetPath}");

        // �򵥴���
        string content = "";
        if (File.Exists(ctx.assetPath))
            content = File.ReadAllText(ctx.assetPath);

        TextAsset asset = new TextAsset(content);
        asset.name = ctx.assetPath;

        ctx.AddObjectToAsset("main", asset);
        ctx.SetMainObject(asset);
    }
}