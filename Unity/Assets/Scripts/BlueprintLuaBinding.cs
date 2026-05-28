// BlueprintLuaBinding.cs
// 将 Blueprint C# API 暴露给第三方 Lua（xLua / tolua / MoonSharp 等）
//
// 架构：
//   Lua 脚本  →  BlueprintLuaAPI（本文件）  →  BPRunner（BlueprintRuntime.cs）  →  C++ Native
//
// 设计原则：
//   · 不依赖任何具体 Lua 框架，通过 ILuaEnv 接口隔离
//   · 使用方先实现适配器（见文件末尾示例），再调用 BlueprintLuaAPI.Register(env, runner)
//   · 注册完成后，Lua 脚本即可使用与内置 Lua VM 完全一致的 Blueprint.* API
//
// Lua 侧用法（与内置 Lua VM 完全对等）：
//
//   Blueprint.RegisterNodeDef({
//     id       = "MyAdd",
//     name     = "My Add",
//     category = "Custom/Math",
//     color    = "FF6600",
//     inputs   = {
//       { name="A", type="Float" },
//       { name="B", type="Float" },
//     },
//     outputs  = {
//       { name="Result", type="Float" },
//     },
//   })
//
//   Blueprint.RegisterHandler("MyAdd", function(ctx)
//     local a = ctx:GetInputFloat("A")
//     local b = ctx:GetInputFloat("B")
//     ctx:SetOutputFloat("Result", a + b)
//     return true
//   end)
//
// 快速接入（xLua 示例，详见文件末尾 XLuaEnvAdapter）：
//
//   var env   = new XLuaEnvAdapter(luaEnv);
//   var api   = new BlueprintLuaAPI(runner);
//   api.Register(env);
//
// ----------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using UnityEngine;

namespace BlueprintRuntime
{
    // =========================================================================
    // ILuaEnv — Lua 环境抽象接口
    // 实现此接口以对接具体的 Lua 框架（xLua / tolua / MoonSharp 等）
    // =========================================================================

    /// <summary>
    /// 抽象的 Lua 环境接口。
    /// 每种 Lua 框架（xLua、tolua、MoonSharp…）提供一个对应的适配器实现。
    /// </summary>
    public interface ILuaEnv
    {
        /// <summary>
        /// 在全局 Lua 环境中注册一个 C# 函数（或对象）。
        /// name 支持点分路径，如 "Blueprint.RegisterNodeDef"。
        /// value 可以是 Delegate、LuaTable 代理对象等，具体类型由框架决定。
        /// </summary>
        void SetGlobal(string name, object value);

        /// <summary>
        /// 读取全局变量。用于初始化时检查 Blueprint 表是否已存在。
        /// </summary>
        object GetGlobal(string name);

        /// <summary>
        /// 将 Lua table（以 Dictionary 表示）解析为托管对象。
        /// 框架适配器负责将自身的 LuaTable 类型转为 Dictionary&lt;string, object&gt;。
        /// </summary>
        Dictionary<string, object> TableToDictionary(object luaTable);

        /// <summary>
        /// 将 Lua array-table（以 List 表示）解析为列表。
        /// </summary>
        List<object> TableToList(object luaTable);
    }

    // =========================================================================
    // LuaExecutionContext — Lua 侧的 ctx 对象（ctx:GetInput / ctx:SetOutput 等）
    // =========================================================================

    /// <summary>
    /// 封装 BPContext，暴露给 Lua 脚本调用的执行上下文。
    /// 方法名与内置 Lua VM 的 ExecutionContext metatable 保持一致。
    /// </summary>
    public sealed class LuaExecutionContext
    {
        private readonly BPContext _ctx;

        internal LuaExecutionContext(BPContext ctx) { _ctx = ctx; }

        // --- 输入引脚 ---
        public int   GetInputInt   (string pin) => _ctx.GetInputInt(pin);
        public long   GetInputLong   (string pin) => _ctx.GetInputLong(pin);
        public double GetInputFloat (string pin) => _ctx.GetInputFloat(pin);
        public bool   GetInputBool  (string pin) => _ctx.GetInputBool(pin);
        public string GetInputString(string pin) => _ctx.GetInputString(pin) ?? "";

        // 兼容 Lua 习惯：统一入口，根据类型自动返回
        public object GetInput(string pin)
        {
            // 先尝试 string，若结果非 null 则为 string 类型
            string s = _ctx.GetInputString(pin);
            if (s != null) return s;
            // 否则返回 double（Lua number 类型）
            return _ctx.GetInputFloat(pin);
        }

        // --- 输出引脚 ---
        public void SetOutputInt   (string pin, int   val) => _ctx.SetOutputInt(pin, val);
        public void SetOutputLong  (string pin, long   val) => _ctx.SetOutputLong(pin, val);
        public void SetOutputFloat (string pin, double val) => _ctx.SetOutputFloat(pin, val);
        public void SetOutputBool  (string pin, bool   val) => _ctx.SetOutputBool(pin, val);
        public void SetOutputString(string pin, string val) => _ctx.SetOutputString(pin, val ?? "");

        // 兼容 Lua 习惯：根据运行时类型自动分派
        public void SetOutput(string pin, object val)
        {
            if (val is string s)          _ctx.SetOutputString(pin, s);
            else if (val is bool b)        _ctx.SetOutputBool(pin, b);
            else if (val is long l)        _ctx.SetOutputLong(pin, l);
            else if (val is int i)         _ctx.SetOutputInt(pin, i);
            else if (val is double d)      _ctx.SetOutputFloat(pin, d);
            else if (val is float f)       _ctx.SetOutputFloat(pin, f);
            else if (val != null)          _ctx.SetOutputString(pin, val.ToString());
        }

        // --- 控制流 ---
        public bool ActivateOutputFlow(string pin) => _ctx.ActivateOutputFlow(pin);

        // --- 变量 ---
        public int   GetVariableInt   (string name) => _ctx.GetVariableInt(name);
        public long   GetVariableLong   (string name) => _ctx.GetVariableLong(name);
        public double GetVariableFloat (string name) => _ctx.GetVariableFloat(name);
        public bool   GetVariableBool  (string name) => _ctx.GetVariableBool(name);
        public string GetVariableString(string name) => _ctx.GetVariableString(name) ?? "";

        public void SetVariableInt   (string name, int   val) => _ctx.SetVariableInt(name, val);
        public void SetVariableLong   (string name, long   val) => _ctx.SetVariableLong(name, val);
        public void SetVariableFloat (string name, double val) => _ctx.SetVariableFloat(name, val);
        public void SetVariableBool  (string name, bool   val) => _ctx.SetVariableBool(name, val);
        public void SetVariableString(string name, string val) => _ctx.SetVariableString(name, val ?? "");

        // --- 日志 / Print ---
        public void Log     (string msg) => _ctx.Log(msg);
        public void LogWarn (string msg) => _ctx.LogWarn(msg);
        public void LogError(string msg) => _ctx.LogError(msg);
        public void Print   (string msg) => _ctx.Print(msg);

        // --- 当前节点信息 ---
        public ulong  CurrentNodeId    => _ctx.CurrentNodeId;
        public string CurrentNodeDefId => _ctx.CurrentNodeDefId;
        public string ActivatedInputPin => _ctx.ActivatedInputPin;
    }

    // =========================================================================
    // BlueprintLuaAPI — 注册 Blueprint.* 到第三方 Lua 环境
    // =========================================================================

    /// <summary>
    /// 将 Blueprint C# API 注册到任意 Lua 环境。
    /// 调用 Register(env) 后，Lua 脚本即可使用 Blueprint.RegisterNodeDef / RegisterHandler 等。
    /// </summary>
    public sealed class BlueprintLuaAPI
    {
        private readonly BPRunner _runner;
        private ILuaEnv _env;

        public BlueprintLuaAPI(BPRunner runner)
        {
            _runner = runner ?? throw new ArgumentNullException(nameof(runner));
        }

        /// <summary>
        /// 将所有 Blueprint.* 函数注册到 Lua 环境。
        /// 必须在 runner 创建后、蓝图加载前调用。
        /// </summary>
        public void Register(ILuaEnv env)
        {
            _env = env ?? throw new ArgumentNullException(nameof(env));

            // Blueprint.RegisterNodeDef(tbl)
            _env.SetGlobal("Blueprint.RegisterNodeDef",
                (Action<object>)LuaRegisterNodeDef);

            // Blueprint.HasNodeDef(id) → bool
            _env.SetGlobal("Blueprint.HasNodeDef",
                (Func<string, bool>)LuaHasNodeDef);

            // Blueprint.RegisterHandler(id, fn)
            // fn 是 Lua function，在各框架里表现为不同类型，统一用 object 接收
            _env.SetGlobal("Blueprint.RegisterHandler",
                (Action<string, object>)LuaRegisterHandler);

            // Blueprint.HasHandler(id) → bool
            _env.SetGlobal("Blueprint.HasHandler",
                (Func<string, bool>)LuaHasHandler);

            // Blueprint.UnregisterHandler(id)
            _env.SetGlobal("Blueprint.UnregisterHandler",
                (Action<string>)LuaUnregisterHandler);
        }

        // ---------------------------------------------------------------------
        // Blueprint.RegisterNodeDef(tbl)
        // tbl 格式与内置 Lua VM 的 Blueprint.RegisterNodeDef 完全一致
        // ---------------------------------------------------------------------

        private void LuaRegisterNodeDef(object luaTable)
        {
            if (_env == null) return;

            Dictionary<string, object> tbl;
            try { tbl = _env.TableToDictionary(luaTable); }
            catch (Exception ex)
            {
                Debug.LogError($"[Blueprint] RegisterNodeDef: failed to parse table — {ex.Message}");
                return;
            }

            if (!tbl.TryGetValue("id", out var idObj) || !(idObj is string id) || string.IsNullOrEmpty(id))
            {
                Debug.LogError("[Blueprint] RegisterNodeDef: 'id' field is required (string)");
                return;
            }

            var def = new BPNodeDef
            {
                id          = id,
                name        = GetString(tbl, "name", id),
                category    = GetString(tbl, "category", ""),
                color       = GetString(tbl, "color", ""),
                description = GetString(tbl, "description", ""),
            };

            var pinList = new List<BPPinDef>();

            // inputs
            if (tbl.TryGetValue("inputs", out var inputsObj))
            {
                foreach (var item in _env.TableToList(inputsObj))
                {
                    var pin = ParsePin(_env.TableToDictionary(item), isInput: true);
                    if (pin != null) pinList.Add(pin);
                }
            }

            // outputs
            if (tbl.TryGetValue("outputs", out var outputsObj))
            {
                foreach (var item in _env.TableToList(outputsObj))
                {
                    var pin = ParsePin(_env.TableToDictionary(item), isInput: false);
                    if (pin != null) pinList.Add(pin);
                }
            }

            def.pins = pinList.ToArray();

            try
            {
                BPRunner.RegisterNodeDef(def);
            }
            catch (Exception ex)
            {
                Debug.LogError($"[Blueprint] RegisterNodeDef '{id}' failed: {ex.Message}");
            }
        }

        private static BPPinDef ParsePin(Dictionary<string, object> p, bool isInput)
        {
            if (p == null) return null;

            var pin = new BPPinDef
            {
                name    = GetString(p, "name", ""),
                isInput = isInput,
                tooltip = GetString(p, "tooltip", ""),
            };

            // isExec 显式字段
            if (p.TryGetValue("isExec", out var isExecObj) && isExecObj is bool ie && ie)
            {
                pin.isExec   = true;
                pin.dataType = BPPinType.Unknown;
                return pin;
            }

            // type 字符串
            string typeStr = GetString(p, "type", "");
            pin.dataType = ParsePinType(typeStr, out bool isExecFromType);
            pin.isExec   = isExecFromType;
            return pin;
        }

        private static BPPinType ParsePinType(string t, out bool isExec)
        {
            isExec = false;
            switch (t)
            {
                case "Flow":    isExec = true; return BPPinType.Unknown;
                case "Boolean": return BPPinType.Boolean;
                case "Integer": return BPPinType.Integer;
                case "Float":   return BPPinType.Float;
                case "String":  return BPPinType.String;
                case "Array":   return BPPinType.Array;
                case "Map":     return BPPinType.Map;
                case "Set":     return BPPinType.Set;
                case "Object":  return BPPinType.Object;
                default:        return BPPinType.Any;
            }
        }

        // ---------------------------------------------------------------------
        // Blueprint.HasNodeDef(id)
        // ---------------------------------------------------------------------

        private bool LuaHasNodeDef(string id) => BPRunner.HasNodeDef(id);

        // ---------------------------------------------------------------------
        // Blueprint.RegisterHandler(id, fn)
        // fn 是第三方 Lua 框架的函数对象，类型因框架而异（ILuaEnv 不统一）
        // 这里要求 ILuaEnv 实现者将函数包装为 Func<LuaExecutionContext, bool>
        // 或直接提供 ILuaCallable 适配（见下方注释）
        // ---------------------------------------------------------------------

        private void LuaRegisterHandler(string id, object luaFunc)
        {
            if (string.IsNullOrEmpty(id) || luaFunc == null) return;

            // luaFunc 由适配器包装为标准 Func<LuaExecutionContext, bool>
            if (luaFunc is Func<LuaExecutionContext, bool> managed)
            {
                BPRunner.RegisterHandler(id, ctx =>
                {
                    try   { return managed(new LuaExecutionContext(ctx)); }
                    catch (Exception ex)
                    {
                        Debug.LogError($"[Blueprint] Lua handler '{id}' threw: {ex.Message}");
                        return false;
                    }
                });
                return;
            }

            // 兼容：适配器传入的是 ILuaHandler（更灵活）
            if (luaFunc is ILuaHandler handler)
            {
                BPRunner.RegisterHandler(id, ctx =>
                {
                    try   { return handler.Call(new LuaExecutionContext(ctx)); }
                    catch (Exception ex)
                    {
                        Debug.LogError($"[Blueprint] Lua handler '{id}' threw: {ex.Message}");
                        return false;
                    }
                });
                return;
            }

            Debug.LogError($"[Blueprint] RegisterHandler '{id}': luaFunc must be " +
                           "Func<LuaExecutionContext,bool> or ILuaHandler. " +
                           "Wrap it in your ILuaEnv adapter.");
        }

        private bool LuaHasHandler(string id) => BPRunner.HasNodeDef(id);

        private void LuaUnregisterHandler(string id) => BPRunner.UnregisterHandler(id);

        // ---------------------------------------------------------------------
        // Helpers
        // ---------------------------------------------------------------------

        private static string GetString(Dictionary<string, object> d, string key, string def)
        {
            return d.TryGetValue(key, out var v) && v is string s ? s : def;
        }
    }

    // =========================================================================
    // ILuaHandler — Lua 函数调用抽象（供框架适配器使用）
    // =========================================================================

    /// <summary>
    /// 封装一个 Lua 函数调用，由框架适配器实现。
    /// 当第三方框架的 Lua 函数类型无法直接转为 Func&lt;...&gt; 时使用。
    /// </summary>
    public interface ILuaHandler
    {
        /// <summary>调用 Lua 函数，返回 true=成功。</summary>
        bool Call(LuaExecutionContext ctx);
    }

    // =========================================================================
    // xLua 适配器示例（需要项目引入 xLua，否则编译时用 #if 排除）
    // 将此代码放入你的项目，或直接继承/修改
    // =========================================================================

#if BLUEPRINT_XLUA  // 在 Player Settings > Scripting Define Symbols 里添加 BLUEPRINT_XLUA

    using XLua;

    /// <summary>
    /// xLua 环境适配器。
    /// 使用方式：
    ///   var adapter = new XLuaEnvAdapter(luaEnv);
    ///   var api = new BlueprintLuaAPI(runner);
    ///   api.Register(adapter);
    /// </summary>
    public sealed class XLuaEnvAdapter : ILuaEnv
    {
        private readonly LuaEnv _env;

        public XLuaEnvAdapter(LuaEnv env)
        {
            _env = env ?? throw new ArgumentNullException(nameof(env));
        }

        public void SetGlobal(string name, object value)
        {
            // 支持点分路径：Blueprint.RegisterNodeDef
            // 先确保 Blueprint table 存在
            string[] parts = name.Split('.');
            if (parts.Length == 2)
            {
                LuaTable tbl = _env.Global.Get<LuaTable>(parts[0]);
                if (tbl == null)
                {
                    _env.DoString($"{parts[0]} = {{}}");
                    tbl = _env.Global.Get<LuaTable>(parts[0]);
                }
                tbl.Set(parts[1], value);
            }
            else
            {
                _env.Global.Set(name, value);
            }
        }

        public object GetGlobal(string name) => _env.Global.Get<object>(name);

        public Dictionary<string, object> TableToDictionary(object luaTable)
        {
            if (luaTable is LuaTable lt)
            {
                var dict = new Dictionary<string, object>();
                lt.ForEach<object, object>((k, v) =>
                {
                    if (k is string ks) dict[ks] = v;
                });
                return dict;
            }
            throw new ArgumentException("Expected LuaTable (xLua), got: " + luaTable?.GetType());
        }

        public List<object> TableToList(object luaTable)
        {
            if (luaTable is LuaTable lt)
            {
                var list = new List<object>();
                int i = 1;
                while (true)
                {
                    var v = lt.Get<int, object>(i);
                    if (v == null) break;
                    list.Add(v);
                    ++i;
                }
                return list;
            }
            throw new ArgumentException("Expected LuaTable (xLua), got: " + luaTable?.GetType());
        }
    }

    /// <summary>
    /// xLua LuaFunction → ILuaHandler 适配器。
    /// 在 ILuaEnv.SetGlobal 注册 RegisterHandler 之前，把 LuaFunction 包装成此类型传入。
    /// </summary>
    public sealed class XLuaHandlerAdapter : ILuaHandler
    {
        private readonly LuaFunction _fn;

        public XLuaHandlerAdapter(LuaFunction fn)
        {
            _fn = fn ?? throw new ArgumentNullException(nameof(fn));
        }

        public bool Call(LuaExecutionContext ctx)
        {
            // xLua: LuaFunction.Call 返回 object[]
            var result = _fn.Call(ctx);
            if (result != null && result.Length > 0 && result[0] is bool b)
                return b;
            return true; // 无返回值视为成功
        }
    }

    // -------------------------------------------------------------------------
    // xLua 专用：覆盖 RegisterHandler 以自动包装 LuaFunction
    // 在项目里替换 BlueprintLuaAPI.Register() 之后调用此扩展
    // -------------------------------------------------------------------------

    public static class XLuaBlueprintExtensions
    {
        /// <summary>
        /// 注册 Blueprint.* 到 xLua 环境，并自动处理 LuaFunction → ILuaHandler 的包装。
        /// 替代 BlueprintLuaAPI.Register(env)。
        /// runner 参数保留是为兼容旧签名；handler 注册现在为进程全局，runner 不参与。
        /// </summary>
        public static void RegisterForXLua(this BlueprintLuaAPI api, LuaEnv luaEnv, BPRunner runner = null)
        {
            var adapter = new XLuaEnvAdapter(luaEnv);
            api.Register(adapter);

            // 覆盖 Blueprint.RegisterHandler，以便直接接受 xLua LuaFunction
            LuaTable blueprint = luaEnv.Global.Get<LuaTable>("Blueprint");
            if (blueprint == null) return;

            blueprint.Set<string, Action<string, LuaFunction>>("RegisterHandler",
                (id, fn) =>
                {
                    BPRunner.RegisterHandler(id, ctx =>
                    {
                        try
                        {
                            var result = fn.Call(new LuaExecutionContext(ctx));
                            if (result != null && result.Length > 0 && result[0] is bool b)
                                return b;
                            return true;
                        }
                        catch (Exception ex)
                        {
                            Debug.LogError($"[Blueprint] xLua handler '{id}' threw: {ex.Message}");
                            return false;
                        }
                    });
                });
        }
    }

#endif // BLUEPRINT_XLUA

    // =========================================================================
    // tolua / toLuaSharp 适配器示例（需要项目引入 tolua，否则用 #if 排除）
    // =========================================================================

#if BLUEPRINT_TOLUA

    using LuaInterface;

    /// <summary>
    /// tolua 环境适配器。
    /// 使用方式：
    ///   var adapter = new ToluaEnvAdapter(luaState);
    ///   var api = new BlueprintLuaAPI(runner);
    ///   api.Register(adapter);
    ///   // 额外调用以处理 LuaFunction：
    ///   ToluaBlueprintExtensions.RegisterForTolua(api, luaState, runner);
    /// </summary>
    public sealed class ToluaEnvAdapter : ILuaEnv
    {
        private readonly LuaState _L;

        public ToluaEnvAdapter(LuaState L)
        {
            _L = L ?? throw new ArgumentNullException(nameof(L));
        }

        public void SetGlobal(string name, object value)
        {
            string[] parts = name.Split('.');
            if (parts.Length == 2)
            {
                // 确保父表存在
                _L.DoString($"if {parts[0]} == nil then {parts[0]} = {{}} end");
                LuaTable tbl = _L.GetTable(parts[0]);
                tbl[parts[1]] = value;
            }
            else
            {
                _L[name] = value;
            }
        }

        public object GetGlobal(string name) => _L[name];

        public Dictionary<string, object> TableToDictionary(object luaTable)
        {
            if (luaTable is LuaTable lt)
            {
                var dict = new Dictionary<string, object>();
                foreach (DictionaryEntry entry in lt)
                {
                    if (entry.Key is string k) dict[k] = entry.Value;
                }
                return dict;
            }
            throw new ArgumentException("Expected LuaTable (tolua), got: " + luaTable?.GetType());
        }

        public List<object> TableToList(object luaTable)
        {
            if (luaTable is LuaTable lt)
            {
                var list = new List<object>();
                for (int i = 1; ; ++i)
                {
                    var v = lt[i];
                    if (v == null) break;
                    list.Add(v);
                }
                return list;
            }
            throw new ArgumentException("Expected LuaTable (tolua), got: " + luaTable?.GetType());
        }
    }

    public static class ToluaBlueprintExtensions
    {
        public static void RegisterForTolua(this BlueprintLuaAPI api, LuaState luaState, BPRunner runner = null)
        {
            var adapter = new ToluaEnvAdapter(luaState);
            api.Register(adapter);

            // 覆盖 Blueprint.RegisterHandler 以直接接受 LuaFunction
            LuaTable blueprint = luaState.GetTable("Blueprint");
            if (blueprint == null) return;

            blueprint["RegisterHandler"] = new Action<string, LuaFunction>((id, fn) =>
            {
                BPRunner.RegisterHandler(id, ctx =>
                {
                    try
                    {
                        object[] result = fn.Call(new LuaExecutionContext(ctx));
                        if (result != null && result.Length > 0 && result[0] is bool b) return b;
                        return true;
                    }
                    catch (Exception ex)
                    {
                        Debug.LogError($"[Blueprint] tolua handler '{id}' threw: {ex.Message}");
                        return false;
                    }
                });
            });
        }
    }

#endif // BLUEPRINT_TOLUA

    // =========================================================================
    // BlueprintBehaviourWithLua — 在 BlueprintBehaviour 基础上加第三方 Lua 支持
    // 继承此类，覆盖 CreateLuaEnv() 返回你的 ILuaEnv 适配器即可
    // =========================================================================

    /// <summary>
    /// 在 BlueprintBehaviour 基础上集成第三方 Lua 支持。
    /// 继承此类并覆盖 CreateLuaEnv() 返回适配器，然后在 RegisterNodes() 中加载 Lua 脚本。
    ///
    /// 示例（xLua）：
    ///
    ///   public class MyBlueprintBehaviour : BlueprintBehaviourWithLua
    ///   {
    ///       public LuaEnv luaEnv;  // 你项目中已存在的 LuaEnv
    ///
    ///       protected override ILuaEnv CreateLuaEnv() => new XLuaEnvAdapter(luaEnv);
    ///
    ///       protected override void RegisterNodes(BPRunner runner)
    ///       {
    ///           base.RegisterNodes(runner);  // 注册 Blueprint.* 到 Lua
    ///           // 执行定义节点的 Lua 脚本
    ///           luaEnv.DoString(@"
    ///               Blueprint.RegisterNodeDef({
    ///                   id = 'MyAdd', name = 'My Add', category = 'Custom',
    ///                   inputs  = { {name='A', type='Float'}, {name='B', type='Float'} },
    ///                   outputs = { {name='Result', type='Float'} },
    ///               })
    ///               Blueprint.RegisterHandler('MyAdd', function(ctx)
    ///                   ctx:SetOutputFloat('Result', ctx:GetInputFloat('A') + ctx:GetInputFloat('B'))
    ///                   return true
    ///               end)
    ///           ");
    ///       }
    ///   }
    /// </summary>
    public abstract class BlueprintBehaviourWithLua : BlueprintBehaviour
    {
        private BlueprintLuaAPI _luaApi;

        /// <summary>返回对接当前 Lua 框架的 ILuaEnv 适配器。</summary>
        protected abstract ILuaEnv CreateLuaEnv();

        protected override void RegisterNodes(BPRunner runner)
        {
            ILuaEnv env = CreateLuaEnv();
            if (env == null)
            {
                Debug.LogError("[Blueprint] CreateLuaEnv() returned null");
                return;
            }
            _luaApi = new BlueprintLuaAPI(runner);
            _luaApi.Register(env);
            // 子类在 base.RegisterNodes() 之后执行 Lua 脚本即可
        }
    }
}
