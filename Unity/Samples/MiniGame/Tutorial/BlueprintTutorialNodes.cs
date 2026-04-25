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
                if (TutorialSystem.Instance == null) { ctx.LogError("TutorialSystem not found"); return false; }
                TutorialSystem.Instance.StartTutorial();
                ctx.ActivateOutputFlow("Out");
                return true;
            });

            runner.RegisterHandler("Tutorial.ShowStep", (ctx) =>
            {
                if (TutorialSystem.Instance == null) return false;
                string id = ctx.GetInputString("StepId");
                TutorialSystem.Instance.ShowStep(id);
                ctx.ActivateOutputFlow("Out");
                return true;
            });

            runner.RegisterHandler("Tutorial.Complete", (ctx) =>
            {
                if (TutorialSystem.Instance == null) return false;
                string id = ctx.GetInputString("StepId");
                TutorialSystem.Instance.CompleteStep(id);
                ctx.ActivateOutputFlow("Out");
                return true;
            });

            runner.RegisterHandler("Tutorial.Check", (ctx) =>
            {
                if (TutorialSystem.Instance == null)
                {
                    ctx.SetOutputBool("IsDone", false);
                    ctx.SetOutputBool("AllDone", false);
                    return false;
                }
                string id = ctx.GetInputString("StepId");
                ctx.SetOutputBool("IsDone", TutorialSystem.Instance.IsStepDone(id));
                ctx.SetOutputBool("AllDone", TutorialSystem.Instance.IsAllDone());
                return true;
            });

            Debug.Log("[BlueprintTutorialNodes] Registered 4 nodes: Tutorial.Start/ShowStep/Complete/Check");
        }
    }
}
