// CocosNPCDemo.ts
// ─────────────────────────────────────────────────────────────────────────────
// Cocos Creator 3.x + Blueprint Runtime 最小接入示例
//
// 场景：点击 NPC → 蓝图驱动 LLM → 显示回复
//
// 挂载方式：把本脚本挂到 NPC 节点上，在 Inspector 里：
//   - 绑定 dialogText：用于显示 NPC 回复的 Label
//   - 绑定 inputEdit：  玩家输入框 EditBox
//   - 绑定 button：    发送按钮 Button
// ─────────────────────────────────────────────────────────────────────────────

import { _decorator, Component, Label, EditBox, Button, resources, TextAsset } from 'cc';
import { BlueprintBridge } from '../blueprint-wx-adapter.js';

const { ccclass, property } = _decorator;

// NPC 对话蓝图（JSON 字符串，可从 resources 加载或硬编码）
// 对应 data/examples/wxgame/NPCDialog.bjson
const NPC_DIALOG_BJSON = `{...}`;  // 构建时用 resources.load 替换

@ccclass('CocosNPCDemo')
export class CocosNPCDemo extends Component {
    @property(Label)   dialogText: Label = null!;
    @property(EditBox) inputEdit:  EditBox = null!;
    @property(Button)  button:     Button = null!;

    /** NPC 性格（蓝图变量 Personality） */
    @property({ displayName: 'NPC 性格' })
    personality: string = '一个活泼好奇的猫咪酒馆老板，说话时喜欢加 "喵~"';

    /** 免费 LLM 接入：智谱 GLM-4-Flash（永久免费） */
    @property({ displayName: 'LLM BaseURL' })
    llmBaseUrl: string = 'https://open.bigmodel.cn/api/paas/v4';

    @property({ displayName: 'LLM ApiKey (智谱免费 Key)' })
    llmApiKey: string = '';  // 去 https://open.bigmodel.cn 注册免费拿

    @property({ displayName: 'LLM 模型' })
    llmModel: string = 'glm-4-flash';

    private runner: any = null;

    async start() {
        this.dialogText.string = '(加载中...)';

        // 1. 初始化 Runtime（只需一次，实际工程里做成单例）
        //    WASM/JS 文件需放到游戏资源目录（remote 加载或内置分包）
        const WASM_URL = 'https://your-cdn.com/BlueprintRuntime.wasm';
        const JS_URL   = 'https://your-cdn.com/BlueprintRuntime.js';

        // 动态 import Emscripten 工厂
        // Cocos Creator 里通常用 resources.load('BlueprintRuntime', TextAsset) + eval 加载
        const BlueprintRuntime: any = (globalThis as any).BlueprintRuntime;
        if (!BlueprintRuntime) {
            this.dialogText.string = '[ERR] BlueprintRuntime.js 未加载';
            return;
        }

        await BlueprintBridge.init({
            wasmUrl: WASM_URL,
            moduleFactory: BlueprintRuntime,
            host: 'wx',   // 小游戏环境走 wx.request
        });

        // 2. 创建 Runner
        this.runner = BlueprintBridge.createRunner();
        this.runner.onPrint((msg: string) => {
            // 蓝图里 PrintString 的输出 → NPC 对话气泡
            this.dialogText.string = msg;
        });
        this.runner.onLog((msg: string, level: number) => {
            if (level >= 2) console.warn('[BP]', msg);
        });

        // 3. 加载对话蓝图
        const ok = this.runner.loadFromJson(NPC_DIALOG_BJSON);
        if (!ok) {
            this.dialogText.string = '[ERR] 蓝图加载失败';
            return;
        }

        // 4. 注入 NPC 初始参数
        this.runner.setVariable('Personality', this.personality);
        this.runner.setVariable('BaseURL',     this.llmBaseUrl);
        this.runner.setVariable('ApiKey',      this.llmApiKey);
        this.runner.setVariable('Model',       this.llmModel);

        // 5. 按钮事件：玩家说话 → 触发蓝图
        this.button.node.on(Button.EventType.CLICK, this.onPlayerSay, this);

        this.dialogText.string = '你好，欢迎来到喵喵酒馆~';
    }

    private onPlayerSay() {
        const input = this.inputEdit.string.trim();
        if (!input) return;

        // 把玩家输入写入蓝图变量，然后执行
        this.runner.setVariable('PlayerInput', input);
        this.runner.execute();

        this.inputEdit.string = '';
        this.dialogText.string = '(思考中...)';
    }

    update(deltaTime: number) {
        // 每帧 Tick —— 推动异步 HTTP 完成回调
        if (this.runner) this.runner.tick(deltaTime);
    }

    onDestroy() {
        if (this.runner) {
            this.runner.destroy();
            this.runner = null;
        }
    }
}
