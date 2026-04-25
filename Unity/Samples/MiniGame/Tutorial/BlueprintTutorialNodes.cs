// ─────────────────────────────────────────────────────────────────────
// BlueprintTutorialNodes.cs — 新手引导蓝图节点
//
// 节点：Tutorial.Start / Tutorial.ShowStep / Tutorial.Complete / Tutorial.Check
// ─────────────────────────────────────────────────────────────────────

using UnityEngine;
using BlueprintRuntime;

namespace BlueprintRuntime.Samples.MiniGame
{
    public static class BlueprintTutorialNodes
    {
        public static void RegisterAll(BPRunner runner)
        {
            runner.RegisterHandler("Tutorial.Start", (ctx) =>
            {
                if (TutorialSystem.Instance == null) { ctx.LogError("TutorialSystem not found"); return; }
                TutorialSystem.Instance.StartTutorial();
                ctx.ActivateOutputFlow("Out");
            });

            runner.RegisterHandler("Tutorial.ShowStep", (ctx) =>
            {
                if (TutorialSystem.Instance == null) return;
                string id = ctx.GetInputString("StepId");
                TutorialSystem.Instance.ShowStep(id);
                ctx.ActivateOutputFlow("Out");
            });

            runner.RegisterHandler("Tutorial.Complete", (ctx) =>
            {
                if (TutorialSystem.Instance == null) return;
                string id = ctx.GetInputString("StepId");
                TutorialSystem.Instance.CompleteStep(id);
                ctx.ActivateOutputFlow("Out");
            });

            runner.RegisterHandler("Tutorial.Check", (ctx) =>
            {
                if (TutorialSystem.Instance == null)
                {
                    ctx.SetOutputBool("IsDone", false);
                    ctx.SetOutputBool("AllDone", false);
                    return;
                }
                string id = ctx.GetInputString("StepId");
                ctx.SetOutputBool("IsDone", TutorialSystem.Instance.IsStepDone(id));
                ctx.SetOutputBool("AllDone", TutorialSystem.Instance.IsAllDone());
            });

            Debug.Log("[BlueprintTutorialNodes] Registered 4 nodes: Tutorial.Start/ShowStep/Complete/Check");
        }
    }
}
