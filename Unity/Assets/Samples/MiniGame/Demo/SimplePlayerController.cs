// SimplePlayerController.cs
// ─────────────────────────────────────────────────────────────────────────────
// 第三人称俯视角控制器 — 适合竖屏小游戏
// WASD/摇杆移动，角色朝移动方向转身，相机固定俯视跟随
// ─────────────────────────────────────────────────────────────────────────────

using UnityEngine;

namespace BlueprintRuntime.Samples.MiniGame.Demo
{
    [RequireComponent(typeof(CharacterController))]
    public class SimplePlayerController : MonoBehaviour
    {
        [Header("移动")]
        [SerializeField] private float moveSpeed = 5f;
        [SerializeField] private float sprintMultiplier = 1.6f;
        [SerializeField] private float gravity = -15f;
        [SerializeField] private float turnSmooth = 0.12f;

        [Header("相机（第三人称俯视）")]
        [SerializeField] private float camHeight = 8f;
        [SerializeField] private float camDistance = 4f;
        [SerializeField] private float camAngle = 55f;           // 俯角
        [SerializeField] private float camFollowSpeed = 8f;

        [Header("交互")]
        [SerializeField] private KeyCode interactKey = KeyCode.E;

        // ── 状态 ────────────────────────────────────────────────────────
        public bool IsInDialog { get; set; }

        private CharacterController _cc;
        private Transform _camTF;
        private float     _verticalVel;
        private float     _turnSmoothVel;

        void Start()
        {
            _cc = GetComponent<CharacterController>();

            // 找相机
            _camTF = GetComponentInChildren<Camera>()?.transform;
            if (_camTF == null && Camera.main != null)
                _camTF = Camera.main.transform;

            // 相机不再是子物体（第三人称跟随）
            if (_camTF != null && _camTF.parent == transform)
                _camTF.SetParent(null);

            // 竖屏光标默认可见（手机端没有鼠标锁定需求）
            Cursor.lockState = CursorLockMode.None;
            Cursor.visible = true;
        }

        void Update()
        {
            if (IsInDialog) return;

            HandleMove();
        }

        void LateUpdate()
        {
            HandleCamera();
        }

        // ── 移动 ────────────────────────────────────────────────────────
        void HandleMove()
        {
            float h = Input.GetAxisRaw("Horizontal");
            float v = Input.GetAxisRaw("Vertical");
            Vector3 input = new Vector3(h, 0, v).normalized;

            // 重力
            if (_cc.isGrounded && _verticalVel < 0)
                _verticalVel = -2f;
            _verticalVel += gravity * Time.deltaTime;

            if (input.magnitude >= 0.1f)
            {
                // 朝移动方向转身
                float targetAngle = Mathf.Atan2(input.x, input.z) * Mathf.Rad2Deg;
                float angle = Mathf.SmoothDampAngle(transform.eulerAngles.y, targetAngle, ref _turnSmoothVel, turnSmooth);
                transform.rotation = Quaternion.Euler(0, angle, 0);

                float speed = moveSpeed * (Input.GetKey(KeyCode.LeftShift) ? sprintMultiplier : 1f);
                Vector3 moveDir = Quaternion.Euler(0, targetAngle, 0) * Vector3.forward;
                _cc.Move((moveDir * speed + Vector3.up * _verticalVel) * Time.deltaTime);
            }
            else
            {
                _cc.Move(Vector3.up * _verticalVel * Time.deltaTime);
            }
        }

        // ── 相机跟随（俯视角） ──────────────────────────────────────────
        void HandleCamera()
        {
            if (_camTF == null) return;

            // 目标位置：玩家后上方
            Vector3 targetPos = transform.position
                + Vector3.up * camHeight
                - Vector3.forward * camDistance;

            _camTF.position = Vector3.Lerp(_camTF.position, targetPos, camFollowSpeed * Time.deltaTime);
            _camTF.rotation = Quaternion.Euler(camAngle, 0, 0);
        }

        // ── 对话模式 ────────────────────────────────────────────────────
        public void EnterDialog()
        {
            IsInDialog = true;
        }

        public void ExitDialog()
        {
            IsInDialog = false;
        }
    }
}
